#if VERSION > 10
#pragma once

#include <set>
#include <sstream>
#include <memory>


#include "llvm.h"
#include "rift.h"

namespace rift {
/**
  The AbstractState represents the abstact state of the function being
  analyzed. It's role is to maintain a mapping between LLVM values and the
  abstract information computed by the analysis.
  Analysis can attach metadata to LLVM values for later use. The metadata
  is not part of the abstract state, but merely associated information for
  later use, e.g. for the optimization.
  */
template <typename AType, typename Metadata = void *>
class AbstractState {
public:
    AType get(llvm::Value * v) {
        if (type.count(v)) return type.at(v);
        changed = true;
        auto n = ATypeBase::B;
        type[v] = n;
        return n;
    }
    
    Metadata getMetadata(llvm::Value * v) {
        if (!metadata.count(v)) return nullptr;
        return metadata.at(v);
    }

    AType initialize(llvm::Value * v, AType t) {
        type[v] = t;
        return t;
    }

    AType update(llvm::Value * v, AType t, Metadata m) {
        metadata[v] = m;
        return update(v, t);
    }

    AType update(llvm::Value * v, AType t) {
        auto prev = get(v);
        if (*ptr(prev) < *ptr(t)) {
            type[v] = t;
            changed = true;
            return t;
        }
        return prev;
    }

    void clear() {
        type.clear();
        metadata.clear();
    }
     
    void erase(llvm::Value * v) {
        metadata.erase(v);
        type.erase(v);
    }

    void iterationStart() {
        changed = false;
    }

    bool hasReachedFixpoint() {
        return !changed;
    }

    AbstractState() : changed(false) {}

    void print(ostream & s) {
        s << "Abstract State: " << "\n";

        struct cmpByName {
            bool operator()(const llvm::Value * a, const llvm::Value * b) const {
                int aName, bName;

                stringstream s;
                llvm::raw_os_ostream ss(s);
                a->printAsOperand(ss, false);
                ss.flush();
                s.seekg(1);
                s >> aName;

                stringstream s2;
                llvm::raw_os_ostream ss2(s2);
                b->printAsOperand(ss2, false);
                ss2.flush();
                s2.seekg(1);
                s2 >> bName;

                return aName < bName;
            }
        };

        set<llvm::Value*, cmpByName> sorted;
        for (auto const & v : type) {
            auto pos = std::get<0>(v);
            sorted.insert(pos);
        }

        for (auto pos : sorted) {
            auto st = type.at(pos);

            llvm::raw_os_ostream ss(s);
            pos->printAsOperand(ss, false);
            ss << ": ";
            if (metadata.count(pos)) {
                ss << * getMetadata(pos) << " ";
            }
            ss.flush();
            s << *st;
            s << endl;
        }
    }

private:
    typedef typename remove_pointer<AType>::type ATypeBase;
    ATypeBase * ptr(ATypeBase & b) { return &b; }
    ATypeBase * ptr(ATypeBase * b) { return b; }

    bool changed;
    map<llvm::Value*, AType> type;
    map<llvm::Value*, Metadata> metadata;
};

} // namespace rift

#endif //VERSION
