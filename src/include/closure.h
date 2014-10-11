#ifndef __CLOSURE_H__
#define __CLOSURE_H__

/*
 * Copyright (C) 2009 - Luca Bigliardi
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <pthread.h>

#include <cstring>
#include <queue>

#include "jutils.h"
#include "exceptions.h"

class Closure;

/*
 * ClosureQueue and ThreadedClosureQueue classes provide a thread-safe queue of Closures.
 *
 * ClosureQueue lets you insert a Closure in the queue and then run all the Closures
 * by hand.
 *
 * ThreadedClosureQueue lets you insert a Closure in the queue; the execution of
 * enqueued Closures is handled automatically by an internal thread.
 *
 */

class ClosureQueue {
public:
    class Error : public FreejError {
public:
        Error(const std::string& msg, int rv)
            : FreejError(msg, rv) {
        }

    };

    ClosureQueue();
    ~ClosureQueue();

    void add_job(Closure *job);
    void do_jobs();

private:
    Closure *get_job_();
    std::queue<Closure *> job_queue_;
    pthread_mutex_t job_queue_mutex_;

};

class ThreadedClosureQueue : ClosureQueue {
public:
    class Error : public FreejError {
public:
        Error(const std::string& msg, int rv)
            : FreejError(msg, rv) {
        }

    };

    class ThreadError : public Error {
public:
        ThreadError(const std::string& msg, int rv)
            : Error(msg, rv) {
        }

    };

    ThreadedClosureQueue();
    ~ThreadedClosureQueue();

    void add_job(Closure *job);

private:
    void signal_();
    static void *jobs_loop_(void *arg);
    bool running_;
    pthread_mutex_t cond_mutex_;
    pthread_cond_t cond_;
    pthread_attr_t attr_;
    pthread_t thread_;

};


/*
 * Closure classes, useful for callbacks and delayed executions of functions
 * or methods (profoundly inspired by ProtocolBuffers' callbacks).
 *
 * Examples:
 * ---- Run a function
 * void function(arg1, arg2) { ... }
 * Closure *closure = NewClosure(&function, arg1, arg2);
 * ...
 * closure.run();
 * delete closure;
 * ---- Run a method
 * class Class {
 *   ...
 *   void method(arg1, arg2);
 *   ...
 * }
 * Class *obj = new Class();
 * Closure *closure = NewClosure(obj, &Class::method, arg1, arg2);
 * ...
 * closure.run();
 * delete closure;
 * ---- Run a method and wait for its execution
 * class Class {
 *   ...
 *   void method(arg1, arg2);
 *   ...
 * }
 * Class *obj = new Class();
 * Closure *closure = NewSyncClosure(obj, &Class::method, arg1, arg2);
 * ... pass closure to another thread who will call closure.run() ...
 * closure.wait();
 * delete closure;
 * ---- Return value (design a function having a retval pointer as argument)
 * void function(arg1, *r);
 * int retval;
 * Closure *closure = NewSyncClosure(&function, arg1, &retval);
 * ...
 *
 * A different number of arguments leads to a different internal representation
 * of a closure to enforce a safe type-checking at compile time. If you need a
 * Closure for a number of arguments not yet covered simply add the appropriate
 * FunctionClosureN MethodClosureN classes, NewClosure() and NewSyncClosure().
 *
 */

class Closure {
public:
    class Error : public FreejError {
public:
        Error(const std::string& msg, int rv)
            : FreejError(msg, rv) {
        }

    };

    Closure(bool synchronized) : synchronized_(synchronized) {
        if(synchronized_) {
            int r;
            if((r = pthread_mutex_init(&cond_mutex_, NULL)) != 0)
                throw Error("Initializing cond_mutex_", r);
            if((r = pthread_cond_init(&cond_, NULL)) != 0)
                throw Error("Initializing cond_", r);
            if((r = pthread_mutex_lock(&cond_mutex_)) != 0)
                throw Error("Preliminary lock of cond_mutex_", r);
        }
    }

    virtual ~Closure() {
        if(synchronized_) {
            int r;
            if((r = pthread_mutex_unlock(&cond_mutex_)) != 0)
                error("In %s , pthread_mutex_unlock(): %s",
                      __PRETTY_FUNCTION__, strerror(r));
            if((r = pthread_cond_destroy(&cond_)) != 0)
                error("In %s , pthread_cond_destroy(): %s",
                      __PRETTY_FUNCTION__, strerror(r));
            if((r = pthread_mutex_destroy(&cond_mutex_)) != 0)
                error("In %s , pthread_mutex_destroy(): %s",
                      __PRETTY_FUNCTION__, strerror(r));
        }
    }

    void run() {
        run_();
        if(synchronized_) {
            int r;
            if((r = pthread_mutex_lock(&cond_mutex_)) != 0)
                throw Error("Pre-signal locking of cond_mutex_", r);
            if((r = pthread_cond_broadcast(&cond_)) != 0)
                throw Error("Signaling cond_", r);
            if((r = pthread_mutex_unlock(&cond_mutex_)) != 0)
                throw Error("Post-signal unlocking of cond_mutex_", r);
        }
    }

    bool is_synchronized() {
        return synchronized_;
    }

    void wait() {
        if(synchronized_) {
            int r;
            if((r = pthread_cond_wait(&cond_, &cond_mutex_)) != 0)
                throw Error("Waiting cond_", r);
        }
    }

private:
    bool synchronized_;
    virtual void run_() = 0;
    pthread_mutex_t cond_mutex_;
    pthread_cond_t cond_;
};

namespace closures {

class FunctionClosure0 : public Closure {
public:
    typedef void (*FunctionType)();

    FunctionClosure0(FunctionType function, bool synchronized)
        : Closure(synchronized),
        function_(function) {
    }

    ~FunctionClosure0();

    void run_() {
        function_();
    }

private:
    FunctionType function_;
};


template <typename Class>
class MethodClosure0 : public Closure {
public:
    typedef void (Class::*MethodType)();

    MethodClosure0(Class* object, MethodType method, bool synchronized)
        : Closure(synchronized),
        object_(object), method_(method) {
    }

    ~MethodClosure0() {
    }

    void run_() {
        (object_->*method_)();
    }

private:
    Class* object_;
    MethodType method_;
};


template <typename Arg1>
class FunctionClosure1 : public Closure {
public:
    typedef void (*FunctionType)(Arg1 arg1);

    FunctionClosure1(FunctionType function, bool synchronized,
                     Arg1 arg1)
        : Closure(synchronized),
        function_(function),
        arg1_(arg1) {
    }

    ~FunctionClosure1() {
    }

    void run_() {
        function_(arg1_);
    }

private:
    FunctionType function_;
    Arg1 arg1_;
};


template <typename Class, typename Arg1>
class MethodClosure1 : public Closure {
public:
    typedef void (Class::*MethodType)(Arg1 arg1);

    MethodClosure1(Class* object, MethodType method, bool synchronized,
                   Arg1 arg1)
        : Closure(synchronized),
        object_(object), method_(method),
        arg1_(arg1) {
    }

    ~MethodClosure1() {
    }

    void run_() {
        (object_->*method_)(arg1_);
    }

private:
    Class* object_;
    MethodType method_;
    Arg1 arg1_;
};


template <typename Arg1, typename Arg2>
class FunctionClosure2 : public Closure {
public:
    typedef void (*FunctionType)(Arg1 arg1, Arg2 arg2);

    FunctionClosure2(FunctionType function, bool synchronized,
                     Arg1 arg1, Arg2 arg2)
        : Closure(synchronized),
        function_(function),
        arg1_(arg1), arg2_(arg2) {
    }

    ~FunctionClosure2() {
    }

    void run_() {
        function_(arg1_, arg2_);
    }

private:
    FunctionType function_;
    Arg1 arg1_;
    Arg2 arg2_;
};


template <typename Class, typename Arg1, typename Arg2>
class MethodClosure2 : public Closure {
public:
    typedef void (Class::*MethodType)(Arg1 arg1, Arg2 arg2);

    MethodClosure2(Class* object, MethodType method, bool synchronized,
                   Arg1 arg1, Arg2 arg2)
        : Closure(synchronized),
        object_(object), method_(method),
        arg1_(arg1), arg2_(arg2) {
    }

    ~MethodClosure2() {
    }

    void run_() {
        (object_->*method_)(arg1_, arg2_);
    }

private:
    Class* object_;
    MethodType method_;
    Arg1 arg1_;
    Arg2 arg2_;
};


template <typename Arg1, typename Arg2, typename Arg3>
class FunctionClosure3 : public Closure {
public:
    typedef void (*FunctionType)(Arg1 arg1, Arg2 arg2, Arg3 arg3);

    FunctionClosure3(FunctionType function, bool synchronized,
                     Arg1 arg1, Arg2 arg2, Arg3 arg3)
        : Closure(synchronized),
        function_(function),
        arg1_(arg1), arg2_(arg2), arg3_(arg3) {
    }

    ~FunctionClosure3() {
    }

    void run_() {
        function_(arg1_, arg2_, arg3_);
    }

private:
    FunctionType function_;
    Arg1 arg1_;
    Arg2 arg2_;
    Arg3 arg3_;
};


template <typename Class, typename Arg1, typename Arg2, typename Arg3>
class MethodClosure3 : public Closure {
public:
    typedef void (Class::*MethodType)(Arg1 arg1, Arg2 arg2, Arg3 arg3);

    MethodClosure3(Class* object, MethodType method, bool synchronized,
                   Arg1 arg1, Arg2 arg2, Arg3 arg3)
        : Closure(synchronized),
        object_(object), method_(method),
        arg1_(arg1), arg2_(arg2), arg3_(arg3) {
    }

    ~MethodClosure3() {
    }

    void run_() {
        (object_->*method_)(arg1_, arg2_, arg3_);
    }

private:
    Class* object_;
    MethodType method_;
    Arg1 arg1_;
    Arg2 arg2_;
    Arg3 arg3_;
};


template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class FunctionClosure4 : public Closure {
public:
    typedef void (*FunctionType)(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4);

    FunctionClosure4(FunctionType function, bool synchronized,
                     Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
        : Closure(synchronized),
        function_(function),
        arg1_(arg1), arg2_(arg2), arg3_(arg3), arg4_(arg4) {
    }

    ~FunctionClosure4() {
    }

    void run_() {
        function_(arg1_, arg2_, arg3_, arg4_);
    }

private:
    FunctionType function_;
    Arg1 arg1_;
    Arg2 arg2_;
    Arg3 arg3_;
    Arg4 arg4_;
};


template < typename Class, typename Arg1, typename Arg2, typename Arg3,
           typename Arg4 >
class MethodClosure4 : public Closure {
public:
    typedef void (Class::*MethodType)(Arg1 arg1, Arg2 arg2, Arg3 arg3,
                                      Arg4 arg4);

    MethodClosure4(Class* object, MethodType method, bool synchronized,
                   Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
        : Closure(synchronized),
        object_(object), method_(method),
        arg1_(arg1), arg2_(arg2), arg3_(arg3), arg4_(arg4) {
    }

    ~MethodClosure4() {
    }

    void run_() {
        (object_->*method_)(arg1_, arg2_, arg3_, arg4_);
    }

private:
    Class* object_;
    MethodType method_;
    Arg1 arg1_;
    Arg2 arg2_;
    Arg3 arg3_;
    Arg4 arg4_;
};


}


inline Closure* NewClosure(void (*function)()) {
    return new closures::FunctionClosure0(function, false);
}

inline Closure* NewSyncClosure(void (*function)()) {
    return new closures::FunctionClosure0(function, true);
}

template <typename Class>
inline Closure* NewClosure(Class* object, void (Class::*method)()) {
    return new closures::MethodClosure0<Class>(object, method, false);
}

template <typename Class>
inline Closure* NewSyncClosure(Class* object, void (Class::*method)()) {
    return new closures::MethodClosure0<Class>(object, method, true);
}

template <typename Arg1>
inline Closure* NewClosure(void (*function)(Arg1),
                           Arg1 arg1) {
    return new closures::FunctionClosure1<Arg1>(function, false, arg1);
}

template <typename Arg1>
inline Closure* NewSyncClosure(void (*function)(Arg1),
                               Arg1 arg1) {
    return new closures::FunctionClosure1<Arg1>(function, true, arg1);
}

template <typename Class, typename Arg1>
inline Closure* NewClosure(Class* object, void (Class::*method)(Arg1),
                           Arg1 arg1) {
    return new closures::MethodClosure1<Class, Arg1>(object, method, false, arg1);
}

template <typename Class, typename Arg1>
inline Closure* NewSyncClosure(Class* object, void (Class::*method)(Arg1),
                               Arg1 arg1) {
    return new closures::MethodClosure1<Class, Arg1>(object, method, true, arg1);
}

template <typename Arg1, typename Arg2>
inline Closure* NewClosure(void (*function)(Arg1, Arg2),
                           Arg1 arg1, Arg2 arg2) {
    return new closures::FunctionClosure2<Arg1, Arg2>(
               function, false, arg1, arg2);
}

template <typename Arg1, typename Arg2>
inline Closure* NewSyncClosure(void (*function)(Arg1, Arg2),
                               Arg1 arg1, Arg2 arg2) {
    return new closures::FunctionClosure2<Arg1, Arg2>(
               function, true, arg1, arg2);
}

template <typename Class, typename Arg1, typename Arg2>
inline Closure* NewClosure(Class* object, void (Class::*method)(Arg1, Arg2),
                           Arg1 arg1, Arg2 arg2) {
    return new closures::MethodClosure2<Class, Arg1, Arg2>(
               object, method, false, arg1, arg2);
}

template <typename Class, typename Arg1, typename Arg2>
inline Closure* NewSyncClosure(
    Class* object, void (Class::*method)(Arg1, Arg2),
    Arg1 arg1, Arg2 arg2) {
    return new closures::MethodClosure2<Class, Arg1, Arg2>(
               object, method, true, arg1, arg2);
}

template <typename Arg1, typename Arg2, typename Arg3>
inline Closure* NewClosure(void (*function)(Arg1, Arg2, Arg3),
                           Arg1 arg1, Arg2 arg2, Arg3 arg3) {
    return new closures::FunctionClosure3<Arg1, Arg2, Arg3>(
               function, false, arg1, arg2, arg3);
}

template <typename Arg1, typename Arg2, typename Arg3>
inline Closure* NewSyncClosure(void (*function)(Arg1, Arg2, Arg3),
                               Arg1 arg1, Arg2 arg2, Arg3 arg3) {
    return new closures::FunctionClosure3<Arg1, Arg2, Arg3>(
               function, true, arg1, arg2, arg3);
}

template <typename Class, typename Arg1, typename Arg2, typename Arg3>
inline Closure* NewClosure(Class* object,
                           void (Class::*method)(Arg1, Arg2, Arg3),
                           Arg1 arg1, Arg2 arg2, Arg3 arg3) {
    return new closures::MethodClosure3<Class, Arg1, Arg2, Arg3>(
               object, method, false, arg1, arg2, arg3);
}

template <typename Class, typename Arg1, typename Arg2, typename Arg3>
inline Closure* NewSyncClosure(Class* object,
                               void (Class::*method)(Arg1, Arg2, Arg3),
                               Arg1 arg1, Arg2 arg2, Arg3 arg3) {
    return new closures::MethodClosure3<Class, Arg1, Arg2, Arg3>(
               object, method, true, arg1, arg2, arg3);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
inline Closure* NewClosure(void (*function)(Arg1, Arg2, Arg3, Arg4),
                           Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
    return new closures::FunctionClosure4<Arg1, Arg2, Arg3, Arg4>(
               function, false, arg1, arg2, arg3, arg4);
}

template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
inline Closure* NewSyncClosure(void (*function)(Arg1, Arg2, Arg3, Arg4),
                               Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
    return new closures::FunctionClosure4<Arg1, Arg2, Arg3, Arg4>(
               function, true, arg1, arg2, arg3, arg4);
}

template < typename Class, typename Arg1, typename Arg2, typename Arg3,
           typename Arg4 >
inline Closure* NewClosure(Class* object,
                           void (Class::*method)(Arg1, Arg2, Arg3, Arg4),
                           Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
    return new closures::MethodClosure4<Class, Arg1, Arg2, Arg3, Arg4>(
               object, method, false, arg1, arg2, arg3, arg4);
}

template < typename Class, typename Arg1, typename Arg2, typename Arg3,
           typename Arg4 >
inline Closure* NewSyncClosure(Class* object,
                               void (Class::*method)(Arg1, Arg2, Arg3, Arg4),
                               Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
    return new closures::MethodClosure4<Class, Arg1, Arg2, Arg3, Arg4>(
               object, method, true, arg1, arg2, arg3, arg4);
}

#endif

// vim:tabstop=2:expandtab:shiftwidth=2

