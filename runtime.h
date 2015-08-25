#ifndef RUNTIME_H
#define RUNTIME_H

/** The Rift runtime manages three kinds of values, function, character
 *  vectors and double vectors. Environments are used to look up variables.
 */

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

struct RVal;

/** Bindings in an environment */
struct Binding {
  void *symbol; 
  RVal *data;
};

/** Environment */
struct Env {
  Env     *parent;   // parent environemtn
  Binding *bindings; // bindings in this environment
  int      size;     // number of bindings in the environment
  int      length;   // length of the binding array
};

Env  *r_env_mk(Env* parent, int length);
RVal *r_env_get(Env* env, void *sym);
Env  *r_env_def(Env* env,  void *sym);
void  r_env_set(Env* env, void* sym, RVal* val);
void  r_env_set_at(Env *env, int at, RVal *val);
RVal *r_env_get_at(Env *env, int at);
void  r_env_del(Env* env);


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

// TODO - what is the type of the code?
// I would have assumed this makes sense
typedef RVal * (*FunPtr)(Env *);

/** Closure */
struct Fun {
  Env *env;
  FunPtr code;
};

Fun *r_fun_mk(Env* env, FunPtr code);

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/** Character vector */
struct CV {
  int   size;
  char *data;
};

CV    *r_cv_mk(int size);
CV    *r_cv_c(int size, ...);
void   r_cv_del(CV *v);
void   r_cv_set(CV *v, int index, char value);
char   r_cv_get(CV *v, int index);
int    r_cv_size(CV *v);
CV    *r_cv_paste(CV *v1, CV *v2);

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

/** Double vector */
struct DV {
  int      size;
  double * data;
};

DV    *r_dv_mk(int size);
DV    *r_dv_c(int size, ...);
void   r_dv_del(DV *v);
void   r_dv_set(DV *v, int index, double value);
double r_dv_get(DV *v, int index);
int    r_dv_size(DV *v);
DV    *r_dv_paste(DV *v1, DV *v2);

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

enum class KIND { FUN, CV, DV };

struct RVal {
  KIND kind;
  union {
    Fun *fun;
    CV  *cv;
    DV  *dv;
  };
};


RVal *r_rv_mk_dv(DV *v);
RVal *r_rv_mk_cv(CV *v);
RVal *r_rv_mk_fun(Fun *v);

DV   *r_rv_as_dv(RVal *v);
CV   *r_rv_as_cv(RVal *v);
Fun  *r_rv_as_fun(RVal *v);

RVal *op_plus(RVal* v1, RVal *v2);
RVal *op_minus(RVal* v1, RVal *v2);
RVal *op_times(RVal* v1, RVal *v2);
RVal *op_divide(RVal* v1, RVal *v2);

// type testing function
int   isa_fun(RVal *v);
int   isa_dv(RVal *v);
int   isa_cv(RVal *v);

// print any value
RVal *print(RVal *v);

// concatenate the two arguments
RVal *paste(RVal *v1, RVal *v2);

// eval
RVal *eval(RVal *v);


#endif // RUNTIME_H

