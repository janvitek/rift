#include <cassert>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <iostream>

#include "runtime.h"

using namespace std;

extern "C" {
/////////////////////////////////////////////////////////////////
// --- Environemnts ---
/////////////////////////////////////////////////////////////////

void RVal::print() {
    switch (kind) {
    case KIND::CV:
        cv->print();
        break;
    case KIND::DV:
        dv->print();
        break;
    case KIND::FUN:
        fun->print();
        break;
    }
}

void DV::print() {
    cout << "double";
    for (int i = 0; i < size; ++i)
        cout << " " << data[i];
    cout << endl;
}

void CV::print() {
    cout << "char";
    for (int i = 0; i < size; ++i)
        cout << " " << data[i];
    cout << endl;
}

void Fun::print() {
    cout << "I am a function" << endl;
}

Env*  r_env_mk(Env* parent, int length) {
  Env *r = new Env();
  r->size = 0;
  r->length = length;
  r->parent = parent;
  r->bindings = new Binding[length];
  return r;
}

RVal* r_env_get(Env* env, void *sym) {
  for (int i = 0; i < env->size; i++) 
    if (env->bindings[i].symbol == sym)
      return env->bindings[i].data;
   if (env->parent != nullptr)
     return r_env_get(env->parent, sym);
   else 
     return nullptr;
}

Env *r_env_def(Env* env,  void *sym) {
  if ( env->length == env->size ) {
    Env *t = r_env_mk( env->parent, env->length + 6);
    for (int i = 0; i < env->size; i++) 
      t->bindings[i] = env->bindings[i];
    delete env;
    env = t;
  }
  for (int i = 0; i < env->size; i ++) 
    if (env->bindings[i].symbol == sym) 
      return env;
  env->size++;
  env->bindings[env->size].symbol = sym;   
  return env;
}

// helper function
static void  env_set(Env* env, void* sym, RVal* val, Env *original) {
  for (int i = 0; i < env->size; i++) 
    if (env->bindings[i].symbol == sym) {
      env->bindings[i].data = val;
      return;
    }
  if ( env->parent != nullptr )
    r_env_set(env->parent, sym, val);
  else {
    r_env_def(original, sym);
    env_set(original, sym, val, nullptr);
  }
}

// if sym is not defined, add to the current env
void  r_env_set(Env* env, void* sym, RVal* val) {
  env_set(env, sym, val, env);
}

// For local variables, access by offset
void  r_env_set_at(Env *env, int at, RVal *val) {
  env->bindings[at].data = val;
}

// For local variables, we can acccess by offset
RVal *r_env_get_at(Env *env, int at) {
  return env->bindings[at].data;
}

void  r_env_del(Env* env) {
  delete env->bindings;
  delete env;
}

///////////////////////////////////////////////////////////////////
//-- Closures
//////////////////////////////////////////////////////////////////

Fun *r_fun_mk(Env *env, FunPtr code) {
  Fun *f = new Fun();
  f->env = env;
  f->code = code;
  return f;
}

//////////////////////////////////////////////////////////////////////
/////-- Character vectors
//////////////////////////////////////////////////////////////////////

CV *r_cv_mk(int size) {
  CV *r = new CV();
  r->data = new char[size + 1];
  r->size = size;
  return r;  
}

CV *r_cv_mk_from_char(char* data) {
  CV *r = new CV();
  int size = strlen(data);
  r->data = new char[size + 1];
  r->size = size;
  strncpy(r->data, data, size + 1);
  return r;  
}

CV *r_cv_c(int size, ...) {
  return nullptr; // TODO - finish
}

void r_cv_del(CV *v) {
  delete v->data;
  delete v;
}
void r_cv_set(CV *v, int index, char value) {
  v->data[index] = value;
}

char r_cv_get(CV *v, int index) {
  return v->data[index];
}

int r_cv_size(CV *v) {
  return v->size;
}

CV *r_cv_paste(CV *v1, CV *v2) { 
  int size = v1->size + v2->size;
  CV *r = new CV();
  r->data = new char[size];
  r->size = size;
  strncpy(r->data, v1->data, v1->size);
  strncpy(r->data+v1->size, v2->data, v2->size);
  return r;
}

//////////////////////////////////////////////////////////////////////
/////-- Double vectors
//////////////////////////////////////////////////////////////////////

DV *r_dv_mk(int size) {
  DV *r = new DV();
  r->data = new double[size];
  r->size = size;
  return r;
}

DV *r_dv_c(int size, ...) {
    va_list ap;
    va_start(ap, size);
    std::vector<DV*> inputs;
    int length = 0;
    for (int i = 0; i <= size; ++i) {
        inputs.push_back(va_arg(ap, DV*));
        length += inputs.back()->size;
    }
    va_end(ap);
    DV * result = r_dv_mk(length);
    size = 0;
    for (DV * dv : inputs) {
        memcpy(result->data + size,dv->data, dv->size);
        size += dv->size;
    }
    return result;
}

void r_dv_del(DV * v) {
  delete v->data;
  delete v;
}

void r_dv_set(DV * v, int index, double value) {
  v->data[index] = value;
}

double r_dv_get(DV * v, int index) {
  return v->data[index];
}

int r_dv_size(DV * v) {
  return v->size;
}

DV *r_dv_paste(DV *v1, DV *v2) {
  int size = v1->size + v2->size;
  DV *r = r_dv_mk(size);
  for(int i = 0; i < v1->size; i++) 
    r->data[i] = v1->data[i];
  for(int i = 0; i < v1->size; i++) 
    r->data[i + v1->size ] = v1->data[i];
  return r;
}

//////////////////////////////////////////////////////////////////////
/////-- RVal
//////////////////////////////////////////////////////////////////////


RVal *r_rv_mk_dv(DV *v) {
  RVal* r = new RVal();
  r->kind = KIND::DV;
  r->dv = v;
  return r;
}

RVal *r_rv_mk_cv(CV *v) {
  RVal* r = new RVal();
  r->kind = KIND::CV;
  r->cv = v;
  return r;
}

RVal *r_rv_mk_fun(Fun *v) {
  RVal* r = new RVal();
  r->kind = KIND::FUN;
  r->fun = v;
  return r;
}

DV   *r_rv_as_dv(RVal *v) {
  return v->dv;
}

CV   *r_rv_as_cv(RVal *v) {
  return v->cv;
}

Fun  *r_rv_as_fun(RVal *v) {
  return v->fun;
}


// helper enum
enum class OP { PLUS, TIMES, DIVIDE, MINUS };

// helper function
static RVal *op_OP(OP op, RVal* v1, RVal *v2) {
  if (v1->kind != v2->kind) {
    return nullptr; // should convert?
  } 
  if ( isa_dv(v1) ) {
    DV *vt1 = v1->dv;
    DV *vt2 = v2->dv;
    if (vt1->size == vt2->size) {
      DV *d = r_dv_mk(vt1->size);
      for( int i = 0; i < vt1->size; i++) {
	switch (op) {
	case OP::PLUS:
	  d->data[i] = vt1->data[i] + vt2->data[i]; break;
	case OP::TIMES:
	  d->data[i] = vt1->data[i] * vt2->data[i]; break;
	case OP::MINUS:
	  d->data[i] = vt1->data[i] - vt2->data[i]; break;
	case OP::DIVIDE:
	  d->data[i] = vt1->data[i] / vt2->data[i]; break;
	}
      }
      return r_rv_mk_dv(d);
    } else {
      if (vt2->size > vt1->size) {
	DV *tmp = vt1;
	vt2 = vt1; 
	vt1 = tmp;
      }
      // reuse vt2
      DV *d = r_dv_mk(vt1->size);
      for( int i = 0, j = 0; i < vt1->size; i++, j++) {
	if ( j == vt2->size) j = 0;
	switch (op) {
	case OP::PLUS:
	  d->data[i] = vt1->data[i] + vt2->data[j]; break;
	case OP::TIMES:
	  d->data[i] = vt1->data[i] * vt2->data[j]; break;
	case OP::MINUS:
	  d->data[i] = vt1->data[i] - vt2->data[j]; break;
	case OP::DIVIDE:
	  d->data[i] = vt1->data[i] / vt2->data[j]; break;
	}
      }
      return r_rv_mk_dv(d);
    }
  } else if ( isa_cv(v1) ) { 
    CV *vt1 = v1->cv;
    CV *vt2 = v2->cv;
    if (vt1->size == vt2->size) {
      CV *d = r_cv_mk(vt1->size);
      for( int i = 0; i < vt1->size; i++) {
	switch (op) {
	case OP::PLUS:
	  d->data[i] = vt1->data[i] + vt2->data[i]; break;
	case OP::TIMES:
	  d->data[i] = vt1->data[i] * vt2->data[i]; break;
	case OP::MINUS:
	  d->data[i] = vt1->data[i] - vt2->data[i]; break;
	case OP::DIVIDE:
	  d->data[i] = vt1->data[i] / vt2->data[i]; break;
	}
      }
      return r_rv_mk_cv(d);
    } else {
      if (vt2->size > vt1->size) {
	CV *tmp = vt1;
	vt2 = vt1; 
	vt1 = tmp;
      }
      // reuse vt2
      CV *d = r_cv_mk(vt1->size);
      for( int i = 0, j = 0; i < vt1->size; i++, j++) {
	if ( j == vt2->size) j = 0;
	switch (op) {
	case OP::PLUS:
	  d->data[i] = vt1->data[i] + vt2->data[j]; break;
	case OP::TIMES:
	  d->data[i] = vt1->data[i] * vt2->data[j]; break;
	case OP::MINUS:
	  d->data[i] = vt1->data[i] - vt2->data[j]; break;
	case OP::DIVIDE:
	  d->data[i] = vt1->data[i] / vt2->data[j]; break;
	}
      }
      return r_rv_mk_cv(d);
    }
  } else {
    return nullptr; // can't add functions
  }
}

RVal *op_plus(RVal* v1, RVal *v2)   { return op_OP(OP::PLUS, v1, v2); }
RVal *op_minus(RVal* v1, RVal *v2)  { return op_OP(OP::MINUS, v1, v2); }
RVal *op_times(RVal* v1, RVal *v2)  { return op_OP(OP::TIMES, v1, v2); }
RVal *op_divide(RVal* v1, RVal *v2) { return op_OP(OP::DIVIDE, v1, v2); }


int  isa_fun(RVal *v) { return v->kind == KIND::FUN; }
int  isa_dv(RVal *v)  { return v->kind == KIND::DV; }
int  isa_cv(RVal *v)  { return v->kind == KIND::CV; }

RVal *print(RVal *v) {
  if ( isa_cv(v) ) {
    cout << v->cv->data << "\n";
  } else if ( isa_dv(v) ) {
    cout << "DV\n";  // TODO: better print
  } else {
    cout << "Fun\n"; // TODO: better print
  }
  return v;
}


RVal *paste(RVal *v1, RVal *v2) {
  if ( v1->kind != v2->kind ) {
    return nullptr; // TODO -- report an error?
  }
  if ( isa_dv(v1) ) {
    return r_rv_mk_dv( r_dv_paste( v1->dv, v2->dv) );
  } else if ( isa_cv(v1) ) {
    return r_rv_mk_cv( r_cv_paste( v1->cv, v2->cv) );
  } else {
    return nullptr; // Adding functions is not defined
  }
}


RVal *eval(RVal *v) {
  return nullptr; // TODO -- how do we do this one?
}

} // extern "C"

// TODO:  is a nullptr  a valid rift value ?  we have no way of writing it.
//        perhaps we should never see that at the user level

// TODO:  there are some runtime functions that are internal to the runtime
//        system;  other can be called by from the user level
//        Should the ones that are user-callable take an environment as 
//        arugment or some kind of context?
