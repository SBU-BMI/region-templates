#include "distTiling.h"

using namespace std;

/*****************************************************************************/
/**                        OpenCV Mat serialization                         **/
/*****************************************************************************/
/*****************************************************************************/

int serializeMat(cv::Mat& m, char* buffer[]) {
    // size = rows * columns * channels * sizeof type
    int size = m.total() * m.elemSize();

    // add number of rows, columns, channels and data type size
    size += 4*sizeof(int);

    // create the buffer
    *buffer = new char[size];

    // add basic fields of a mat
    int rows = m.rows;
    int cols = m.cols;
    int type = m.type();
    int channels = m.channels();

    memcpy(&(*buffer)[0*sizeof(int)], &rows, sizeof(int));
    memcpy(&(*buffer)[1*sizeof(int)], &cols, sizeof(int));
    memcpy(&(*buffer)[2*sizeof(int)], &type, sizeof(int));
    memcpy(&(*buffer)[3*sizeof(int)], &channels, sizeof(int));

    // mat data need to be contiguous in order to use memcpy
    if(!m.isContinuous()){ 
        m = m.clone();
    }

    // copy actual data to buffer
    memcpy(&(*buffer)[4*sizeof(int)], m.data, m.total()*m.channels());

    return size;
}

void deserializeMat(cv::Mat& m, char buffer[]) {
    // get the basic fields of the mat
    int rows, cols, type, channels;
    memcpy(&rows, &(buffer[0*sizeof(int)]), sizeof(int));
    memcpy(&cols, &(buffer[1*sizeof(int)]), sizeof(int));
    memcpy(&type, &(buffer[2*sizeof(int)]), sizeof(int));
    memcpy(&channels, &(buffer[3*sizeof(int)]), sizeof(int));//NOT USING YET?

    // create the new mat with the basic fields and the actual data
    m = cv::Mat(rows, cols, type, &(buffer[4*sizeof(int)]));
}

/*****************************************************************************/
/**                         Communication routines                          **/
/*****************************************************************************/
/*****************************************************************************/

// pops the first element of a list, returning 0 if the list is empty
template <typename T>
int pop(std::list<T>* l, T& out) {
    if (l->size() == 0)
        return 0;
    else {
        out = *(l->begin());
        l->erase(l->begin());
        return 1;
    }
}

int recvTile(cv::Mat& tile, int rank, char* buffer[]) {
    // get the mat object size
    // std::cout << "[" << rank << "][recvTile] Waiting " << std::endl;
    int bufSize = 0;
    MPI_Recv(&bufSize, 1, MPI_INT, MPI_MANAGER_RANK, 
        MPI_TAG, MPI_COMM_WORLD, NULL);

    // std::cout << "[" << rank << "][recvTile] received size " 
    //     << bufSize << std::endl;

    // get the actual data, if there is any
    if (bufSize > 0) {
        *buffer = new char[bufSize];

        MPI_Recv(*buffer, bufSize, MPI_UNSIGNED_CHAR, 
            MPI_MANAGER_RANK, MPI_TAG, MPI_COMM_WORLD, NULL);
        // std::cout << "[" << rank << "][recvTile] Received tile " << std::endl;

        // generate the output mat from the received data
        deserializeMat(tile, *buffer);
        // std::cout << "[" << rank << "][recvTile] Tile deserialized" 
        //     << std::endl;
    }

    return bufSize;
}

void* sendRecvThread(void *args) {
    rect_t r;
    int currentRank = ((thr_args_t*)args)->currentRank;
    cv::Mat* input = ((thr_args_t*)args)->input;
    std::list<rect_t>* rQueue = ((thr_args_t*)args)->rQueue;

    std::cout << "[" << currentRank << "][sendRecvThread] Manager " 
        << "thread started" << std::endl;

    // keep getting new rect's from the queue
    while (pop(rQueue, r) != 0) {
        // generate a submat from the rect's queue
        cv::Mat subm = input->colRange(r.xi, r.xo).rowRange(r.yi, r.yo);
        
        // serialize the mat
        char* buffer;
        int bufSize = serializeMat(subm, &buffer);

        // send the tile size and the tile itself
        MPI_Send(&bufSize, 1, MPI_INT, currentRank, MPI_TAG, MPI_COMM_WORLD);
        MPI_Send(buffer, bufSize, MPI_UNSIGNED_CHAR, 
            currentRank, MPI_TAG, MPI_COMM_WORLD);

        // wait for the resulting mat
        // obs: the same sent buffer is used for receiving since they must
        //   have the same size.
        MPI_Recv(buffer, bufSize, MPI_UNSIGNED_CHAR, 
            currentRank, MPI_TAG, MPI_COMM_WORLD, NULL);

        // make a mat object from the returned buffer data
        cv::Mat resultm;
        deserializeMat(resultm, buffer);

        // copy result data back to input
        // NOT THREAD-SAFE (ok because there is no overlap)
        resultm.copyTo((*input)(cv::Rect(r.xi, r.yi, r.xo-r.xi, r.yo-r.yi)));

        delete[] buffer;
    }

    std::cout << "[" << currentRank << "][sendRecvThread] Manager " 
        << "thread has no more tiles" << std::endl;

    // send final empty message, signaling the end
    int end = 0;
    MPI_Send(&end, 1, MPI_INT, currentRank, MPI_TAG, MPI_COMM_WORLD);

    return NULL;
}

/*****************************************************************************/
/**                              MPI processes                              **/
/*****************************************************************************/
/*****************************************************************************/

void managerProc(int rank, int np, cv::Mat& inImg) {
    std::list<rect_t> rQueue = autoTiler(inImg);
    std::cout << "[" << rank << "][distExec] Starting manager with " 
        << rQueue.size() << " tiles" << std::endl;

    // create a send/recv thread for each worker
    pthread_t threadsId[np-1];
    for (int p=1; p<np; p++) {
        thr_args_t* args = new thr_args_t();
        args->currentRank = p;
        args->input = &inImg;
        args->rQueue = &rQueue;
        pthread_create(&threadsId[p], NULL, sendRecvThread, args);
    }

    std::cout << "[" << rank << "][distExec] Manager waiting "
        << "for comm threads to finish\n";

    // wait for all threads to finish
    for (int p=1; p<np; p++) {
        pthread_join(threadsId[p], NULL);
    }
    std::cout << "[" << rank << "][distExec] " 
        << "Manager threads done" << std::endl;
}

void workerProc(int rank) {
    // receive either a new tile or an empty tag, signaling the end
    cv::Mat curTile;
    cv::Mat outTile;

    // keep processing tiles while there are tiles
    // recvTile returns the tile size, returning 0 if the
    //   tile is empty, signaling that there are no more tiles
    int bufSize = 0;
    // std::cout << "[" << rank << "][distExec] Waiting new tile" << std::endl;
    char* recvBuffer;
    while ((bufSize = recvTile(curTile, rank, &recvBuffer)) > 0) {
        // std::cout << "[" << rank << "][distExec] Got tile sized :" 
        //     << bufSize << std::endl;

        // allocate halide buffers
        Halide::Runtime::Buffer<uint8_t> h_curTile = 
            Halide::Runtime::Buffer<uint8_t>(
            curTile.data, curTile.cols, curTile.rows);
        outTile = cv::Mat::zeros(curTile.size(), curTile.type());
        Halide::Runtime::Buffer<uint8_t> h_outTile = 
            Halide::Runtime::Buffer<uint8_t>(
            outTile.data, outTile.cols, outTile.rows);

        // execute blur 
        blurAOT(h_curTile, h_outTile);
        std::cout << "[" << rank << "][distExec] Executed tile sized: "
            << bufSize << std::endl;
        
        // serialize the output tile
        char* buffer;
        serializeMat(outTile, &buffer);

        // send it back to the manager
        // std::cout << "[" << rank << "][distExec] Sending result" 
        //     << std::endl;
        MPI_Send(buffer, bufSize, MPI_UNSIGNED_CHAR, 
            MPI_MANAGER_RANK, MPI_TAG, MPI_COMM_WORLD);
        // std::cout << "[" << rank << "][distExec] Waiting new tile" 
        //     << std::endl;

        delete[] recvBuffer;
        delete[] buffer;
    }
}

/*****************************************************************************/
/**                              Main routine                               **/
/*****************************************************************************/
/*****************************************************************************/

int distExec(int argc, char* argv[], cv::Mat& inImg) {

    int np, rank;

    // initialize mpi and separate manager from worker nodes
    if (MPI_Init(&argc, &argv) != 0) {
        cout << "[distExec] Init error" << endl;
        exit(1);
    }

    if (MPI_Comm_size(MPI_COMM_WORLD, &np) != 0 
            || MPI_Comm_rank(MPI_COMM_WORLD, &rank) != 0) {
        cout << "[distExec] Rank error" << endl;
        exit(1);
    }

    // node 0 is the manager
    if (rank == 0) {
        managerProc(rank, np, inImg);
        cv::imwrite("./output.png", inImg);
    } else {
        std::cout << "[" << rank << "][distExec] Starting worker" << std::endl;
        workerProc(rank);
        std::cout << "[" << rank << "][distExec] Worker finished" << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD); 
    MPI_Finalize();

    return 0;
}
