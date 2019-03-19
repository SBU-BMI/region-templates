IN=$1
./disTiler -i $IN -d 0
xdg-open $IN
xdg-open output.png