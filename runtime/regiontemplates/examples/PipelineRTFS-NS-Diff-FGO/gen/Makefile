all:
	@cd json
	@g++ -c json/jsoncpp.cpp -std=c++11 -o jsoncpp
	@g++ gen.cpp jsoncpp -std=c++11 -o gen.out
	@./gen.out -d segmentation_desc
	# @./gen.out -d normalization_desc
	@mv Segmentation.* ../stages
	# @mv NormalizationComp.* ..
debug:
	g++ -g -ggdb -gdwarf-2 gen.cpp jsoncpp -std=c++11 -o gen.out
gen:
	@./gen.out -d segmentation_desc
	# @./gen.out -d normalization_desc
	@mv Segmentation.* ../stages
	# @mv NormalizationComp.* ..