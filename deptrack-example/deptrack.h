#pragma once
#include <memory>
#include <iostream>
#include <list>
#include <cassert>
#include <utility>
#include "tracking.h"
#include "../extras"

template<typename T, Tracking::TrackingId tid>
class ReadVal;

template<typename T, Tracking::TrackingSet... permitted>
class WriteVal;

template<typename T, Tracking::TrackingId s, Tracking::TrackingSet... sets>
struct TransVals;



template<typename T, Tracking::TrackingSet st>
class IntermVal{
private:
	
	T internal;
	
	IntermVal(T &&t):internal(std::move(t)),r(*this){}
	static constexpr long long s = st;

	template<typename T_, Tracking::TrackingSet s>
	static constexpr bool not_intermval_f(IntermVal<T_,s>*) { return false; }

	template<typename T_>
	static constexpr bool not_intermval_f(T_*) { return true; }

	template<typename T_>
	struct not_intermval : public std::integral_constant<bool,not_intermval_f( (T_*) nullptr )>::type {};

public:

	IntermVal &r;

	template<typename T_, Tracking::TrackingSet s_>
	auto touch(IntermVal<T_, s_> &&t){
		using namespace Tracking;
		constexpr auto v = combine(s, sub(s_,s));
		return (IntermVal<T_, v>) std::move(t);
	}

	template<typename T_, Tracking::TrackingId... ids>
	auto touch(WriteVal<T_,ids...> &&t) {
		using namespace Tracking;
		static_assert(contains(combine(ids...),st), "Invalid indirect flow detected!");
		return t;
	}

	template<typename T_, Tracking::TrackingId id, Tracking::TrackingId... ids>
	auto touch(TransVals<T_,id,ids...> &&t) {
		using namespace Tracking;
		static_assert(contains(combine(id,ids...),st), "Invalid indirect flow detected!");
		return t;
	}


	template<Tracking::TrackingId id>
	IntermVal(const IntermVal<T,id> &rv):internal(rv.internal),r(*this){}

#define allow_op(op) \
	template<Tracking::TrackingSet s_> \
	auto operator op (IntermVal<T,s_> v){ \
		auto tmp = this->internal  op  v.internal; \
		return IntermVal<decltype(tmp), Tracking::combine(s,s_)> (std::move(tmp)); \
	} \
\
	template<typename T_> \
	auto operator op (T_ v){ \
		static_assert(not_intermval<T_>::value, "Hey!  that's cheating!"); \
		auto tmpres = this->internal  op  v; \
		return IntermVal<decltype(tmpres),s> (std::move(tmpres)); \
	} \

	allow_op(-)
	allow_op(+)
	allow_op(*)
	allow_op(/)
	allow_op(==)
	allow_op(<)
	allow_op(>)

	template<typename F>
	auto f(F g){
		static_assert(is_stateless<F, T>::value, "No cheating!");
		auto res = g(this->internal);
		return IntermVal<decltype(res), s>(std::move(res));
	}



	template<typename F, typename G, typename... Args>
	//typename std::enable_if <is_stateless<F, strouch<Args>::type...>::value && is_stateless<G, stouch<Args>::type...>::value >::type
	void
	ifTrue(F f, G g, Args... rest) {
		//rest should be exact items we wish to use in the subsequent computation.
		//will just cast them all to themselves + this type
		if (this->internal) f(touch(std::move(rest))...);
		else g(touch(std::move(rest))...);
	}

	template<typename T_, Tracking::TrackingId tid>
	friend class ReadVal;

	template<typename T_, Tracking::TrackingSet s_>
	friend class IntermVal;

	template<typename T_, Tracking::TrackingSet... s_>
	friend class WriteVal;

	template<typename T_, Tracking::TrackingId s_, Tracking::TrackingSet... >
	friend struct TransVals;

	//LOCAL ONLY.  FOR PROTOTYPING.
	
	T localVal(){ return this->internal;}

	void display(){
		std::cout << this->internal << std::endl;
	}

	static void displaySources(){
		for (auto e : Tracking::asList(s))
			std::cout << e << ",";
		std::cout << std::endl;
	}

};

template<typename T, Tracking::TrackingId tid>
class ReadVal : public IntermVal<T, tid>{

public:
	ReadVal(T &&t):IntermVal<T,tid>(std::move(t)){}
	static constexpr Tracking::TrackingId id() { return tid;}
	operator IntermVal<T,tid> () {return *this;}
};

template<typename T, Tracking::TrackingSet... permitted>
class WriteVal {
private:
	T& internal;
public:

	WriteVal(T &t):internal(t){}

	static constexpr Tracking::TrackingSet permset = Tracking::combine(permitted...);

	template<Tracking::TrackingSet cnds>
	void add(IntermVal<T, cnds>){
		static_assert(Tracking::subset(permset,cnds), "Error: id not allowed! Invalid Flow!");
	}
	template<Tracking::TrackingSet cnds>
	void put(IntermVal<T, cnds>&){
		static_assert(Tracking::subset(permset,cnds), "Error: id not allowed! Invalid Flow!");
	}
	template<Tracking::TrackingSet cnds>
	void put(IntermVal<T, cnds>&&){
		static_assert(Tracking::subset(permset,cnds), "Error: id not allowed! Invalid Flow!");
	}

	void incr(){}

	template<typename F, typename T_, Tracking::TrackingSet cnds>
	void c(F g, IntermVal<T_, cnds> &iv){
		static_assert(Tracking::subset(permset,cnds), "invalid flow");
		g(this->internal, iv.internal);
	}
};


#define IDof(a) std::decay<decltype(a)>::type::id()

template<typename T, Tracking::TrackingId s, Tracking::TrackingSet... sets>
struct TransVals : public ReadVal<T,s>, public WriteVal<T,s,sets...>, 
		   public std::pair<ReadVal<T,s>&, WriteVal<T,s,sets...>& >
{

	typedef T t;
	ReadVal<T,s>& r() {return this->first;}
	WriteVal<T,s,sets...>& w() {return this->second;}
	IntermVal<T,s>& i() {return r();}

	TransVals(T &&init_val):
		ReadVal<T,s>(std::move(init_val)),
		WriteVal<T,s,sets...>(this->ReadVal<T,s>::internal),
		std::pair<ReadVal<T,s>&, WriteVal<T,s,sets...>& >(*this,*this){}
};

#define TVparams typename __T___, Tracking::TrackingId __s___, Tracking::TrackingSet... __sets___
#define TransValsA TransVals<__T___,__s___,__sets___...>

#define TranVals(T, ids...) TransVals<T,gen_id(), ##ids>

//so it would be really cool to hack clang and all, but I think that
//these macros will work fine for the prototype stages.

#define TIF3(a,f,g,b, c, d) {						\
		a.ifTrue([](decltype(a.touch(std::move(b))) b, \
			    decltype(a.touch(std::move(d))) d,		\
			    decltype(a.touch(std::move(c))) c) {f}	\
			 ,[](decltype(a.touch(std::move(b))) b, \
			     decltype(a.touch(std::move(d))) d, \
			     decltype(a.touch(std::move(c))) c) {g}, b, d, c ); }


#define TIF2(a,f,g,b, c) {						\
		a.ifTrue([](decltype(a.touch(std::move(b))) b, \
			    decltype(a.touch(std::move(c))) c) {f}	\
			 ,[](decltype(a.touch(std::move(b))) b, \
			     decltype(a.touch(std::move(c))) c) {g}, b, c ); }

#define TIF(a,f,g,b) {						\
		a.ifTrue([](decltype(a.touch(std::move(b))) b) {f},	\
			 [](decltype(a.touch(std::move(b))) b) {g}, b); }
