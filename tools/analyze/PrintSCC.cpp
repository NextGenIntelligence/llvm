//===- PrintSCC.cpp - Enumerate SCCs in some key graphs -------------------===//
//
// This file provides passes to print out SCCs in a CFG or a CallGraph.
// Normally, you would not use these passes; instead, you would use the
// TarjanSCCIterator directly to enumerate SCCs and process them in some way.
// These passes serve three purposes:
// (1) As a reference for how to use the TarjanSCCIterator.
// (2) To print out the SCCs for a CFG or a CallGraph:
//       analyze -cfgscc            to print the SCCs in each CFG of a module.
//       analyze -cfgscc -stats     to print the #SCCs and the maximum SCC size.
//       analyze -cfgscc -debug > /dev/null to watch the algorithm in action.
// 
//     and similarly:
//       analyze -callscc [-stats] [-debug] to print SCCs in the CallGraph
// 
// (3) To test the TarjanSCCIterator.
//
//===----------------------------------------------------------------------===//

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/CFG.h"
#include "Support/TarjanSCCIterator.h"

namespace {
struct CFGSCC: public FunctionPass {
  bool runOnFunction(Function& func) {
    unsigned long sccNum = 0;
    std::cout << "SCCs for Function " << func.getName() << " in PostOrder:";
    for (TarjanSCC_iterator<Function*> I = tarj_begin(&func),
           E = tarj_end(&func); I != E; ++I) {
      SCC<Function*> &nextSCC = **I;
      std::cout << "\nSCC #" << ++sccNum << " : ";
      for (SCC<Function*>::const_iterator I = nextSCC.begin(),E = nextSCC.end();
           I != E; ++I)
        std::cout << (*I)->getName() << ", ";
      if (nextSCC.size() == 1 && nextSCC.HasLoop())
        std::cout << " (Has self-loop).";
    }
    std::cout << "\n";

    return true;
  }
  void print(std::ostream &O) const { }
};


struct CallGraphSCC : public Pass {
  // run - Print out SCCs in the call graph for the specified module.
  bool run(Module &M) {
    CallGraphNode* rootNode = getAnalysis<CallGraph>().getRoot();
    unsigned long sccNum = 0;
    std::cout << "SCCs for the program in PostOrder:";
    for (TarjanSCC_iterator<CallGraphNode*> SCCI = tarj_begin(rootNode),
         E = tarj_end(rootNode); SCCI != E; ++SCCI) {
      const SCC<CallGraphNode*> &nextSCC = **SCCI;
      std::cout << "\nSCC #" << ++sccNum << " : ";
      for (SCC<CallGraphNode*>::const_iterator I = nextSCC.begin(),
             E = nextSCC.end(); I != E; ++I)
        std::cout << ((*I)->getFunction() ? (*I)->getFunction()->getName()
                      : std::string("Indirect CallGraph node")) << ", ";
      if (nextSCC.size() == 1 && nextSCC.HasLoop())
        std::cout << " (Has self-loop).";
    }
    std::cout << "\n";

    return true;
  }

  void print(std::ostream &O) const { }

  // getAnalysisUsage - This pass requires the CallGraph.
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<CallGraph>();
  }
};

  RegisterAnalysis<CFGSCC>
  Y("cfgscc", "Print SCCs of each function CFG");

  RegisterAnalysis<CallGraphSCC>
  Z("callscc", "Print SCCs of the Call Graph");
}
