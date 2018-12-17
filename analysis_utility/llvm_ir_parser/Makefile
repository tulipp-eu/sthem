CFLAGS = -std=gnu++11 -fno-rtti -O -g `${LLVM_CONFIG} --cxxflags`
LDFLAGS = -Wl,--start-group -lclangAST -lclangASTMatchers -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangEdit -lclangRewrite -lclangRewriteFrontend -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangSerialization -lclangToolingCore -lclangTooling -lclangFormat -Wl,--end-group `${LLVM_CONFIG} --ldflags --libs --system-libs`

###############################################################################

default : llvm_ir_parser

###############################################################################

cfg.o : src/cfg.cpp src/cfg.h
	${CLANGPP} ${CFLAGS} -c $< -o $@

llvm_ir_parser.o : src/llvm_ir_parser.cpp src/cfg.h
	${CLANGPP} ${CFLAGS} -c $< -o $@

llvm_ir_parser : llvm_ir_parser.o cfg.o
	${CLANGPP} $^ ${LDFLAGS} -o $@

.PHONY : clean
clean :
	rm -rf *.o llvm_ir_parser *.ll *.xml
