tulipp_source_tool : tulipp_source_tool.cpp
	${CLANGPP} -fno-rtti -O -g `${LLVM_CONFIG} --cxxflags` $< -Wl,--start-group -lclangAST -lclangASTMatchers -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangEdit -lclangRewrite -lclangRewriteFrontend -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangSerialization -lclangToolingCore -lclangTooling -lclangFormat -Wl,--end-group `${LLVM_CONFIG} --ldflags --libs --system-libs` -o $@

.PHONY : clean
clean :
	rm -rf tulipp_source_tool

