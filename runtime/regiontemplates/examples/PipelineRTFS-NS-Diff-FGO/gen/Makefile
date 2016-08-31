all:
	g++ gen.cpp jsoncpp -std=c++11 -o gen
	./gen -d segmentation_desc
	# ./gen -d normalization_desc
	mv Segmentation.* ..
	# mv NormalizationComp.* ..
debug:
	g++ -g -ggdb -gdwarf-2 gen.cpp jsoncpp -std=c++11 -o gen
gen:
	./gen -d segmentation_desc
	# ./gen -d normalization_desc
	mv Segmentation.* ..
	# mv NormalizationComp.* ..