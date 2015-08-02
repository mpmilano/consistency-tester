#pragma once

#include "Transaction.hpp"
#include "Operation.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <iostream>
#include <fstream>
#include <set>
#include <cstdlib>

template<Level l>
struct FileStore {
	template<typename T>
	struct FSObject : public RemoteObject<T> {
		std::unique_ptr<T> t;
		const std::string filename;
		
		FSObject(const std::string &name, bool exists = false):filename(name){
			if (!exists){
				std::ofstream ofs(filename);
				boost::archive::text_oarchive oa(ofs);
				FSObject<T> &ths = *this;
				oa << ths;
			}
		}
		
		FSObject(const std::string &name, const T &init):t(heap_copy(init)),filename(name) {
			std::ofstream ofs(filename);
			boost::archive::text_oarchive oa(ofs);
			static_assert(!std::is_const<decltype(this)>::value);
			FSObject<T> &ths = *this;
			oa << ths;
		}

		struct StupidWrapper{
			typename std::conditional<std::is_default_constructible<T>::value, T, T*>::type val;

			template<typename T2, restrict(std::is_same<decltype(val) CMA T2>::value)>
			StupidWrapper(const T2 &t):val(t) {}

			template<typename T2, restrict2(!std::is_same<decltype(val) CMA T2>::value)>
			StupidWrapper(const T2 &t):val(*t) {}

			StupidWrapper(){}

			template<typename T2>
			static const auto& deref(const T2 *t2) {
				return *t2;
			}

			template<typename T2, restrict(!std::is_pointer<T2>::value)>
			static const auto& deref(const T2 &t2) {
				return t2;
			}

			auto deref() const {
				return deref(val);
			}

		};

		template<class Archive> typename std::enable_if<std::is_pod<T>::value &&
		!std::is_pod<Archive>::value >::type
		save(Archive &ar, const uint) const {
			StupidWrapper stupid(t.get());
			ar & stupid.val;
		}

		template<class Archive> typename std::enable_if<std::is_pod<T>::value &&
		!std::is_pod<Archive>::value >::type
		load(Archive &ar, const uint){
			StupidWrapper stupid;
			ar & stupid.val;
			t.reset(heap_copy(stupid.deref()));
		}
		
		BOOST_SERIALIZATION_SPLIT_MEMBER()

		template<class Archive> typename std::enable_if<!std::is_pod<T>::value &&
		!std::is_pod<Archive>::value>::type
		save(Archive &, const uint) const {
			assert(false && "unimplemented");
		}

		
		template<class Archive> typename std::enable_if<!std::is_pod<T>::value &&
		!std::is_pod<Archive>::value>::type
		load(Archive &, const uint) const {
			assert(false && "unimplemented");
		}

		virtual const T& get() const {
			std::ifstream ifs(filename);
			boost::archive::text_iarchive ia(ifs);
			ia >> *const_cast<FSObject<T>*>(this);
			return *t;
		}

		virtual void put(const T& t) {
			std::ofstream ofs(filename);
			boost::archive::text_oarchive oa(ofs);
			this->t.reset(heap_copy(t));
			oa << *this;
		}
	};
	
	template<typename T>
	struct FSDir : public FSObject<std::set<T> > {
		FSDir(const std::string &name):FSObject<std::set<T> >(name,true){
			system(("exec mkdir -p " + name).c_str());
		}

		template<class Archive>
		void serialize(Archive &, const uint){
			assert(false && "this should not be serialized");
		}

		const std::set<T>& get() const {
			static std::set<T> ret;
			ret.clear();
			for (const auto &str : read_dir(this->filename)){
				FSObject<T> obj(this->filename + str,true);
				ret.insert(obj.get() );
			}
			return ret;
		}

		virtual void put(const std::set<T> &s) {
			std::system(("exec rm -r " + this->filename + "*").c_str());
			for (const auto &e : s){
				FSObject<T> obj(this->filename + std::to_string(gensym()) );
				obj.put(e);
			}
		}
	};

	template<HandleAccess ha, typename T>
	auto newObject(){
		return make_handle
			<l,ha,T,FSObject<T>>
			(std::string("/tmp/fsstore/") + std::to_string(gensym()));
	}

	template<HandleAccess ha, typename T>
	auto newCollection(){
		return make_handle
			<l,ha,std::set<T>,FSDir<T>>
			(std::string("/tmp/fsstore/") + std::to_string(gensym()) + "/");
	}

	template<HandleAccess ha, typename T>
	auto newObject(const T &t){
		return make_handle
			<l,ha,T,FSObject<T>>
			(std::string("/tmp/fsstore/") + std::to_string(gensym()),t);
	}

	template<typename T>
	static auto tryCast(RemoteObject<T>* r){
		if(auto *ret = dynamic_cast<FSObject<T>* >(r))
			return ret;
		else throw Transaction::ClassCastException();
	}

	template<typename T, restrict(!is_RemoteObj_ptr<T>::value)>
	static auto tryCast(T && r){
		return std::forward<T>(r);
	}
	
};

typedef FileStore<Level::strong> StrongFileStore;
typedef FileStore<Level::causal> WeakFileStore;

template<Level l, typename T>
using FSObject = typename FileStore<l>::template FSObject<T>;

template<Level l, typename T>
using FSDir = typename FileStore<l>::template FSDir<T>;
	
template<typename T, typename E>
DECLARE_OPERATION(Insert, RemoteObject<std::set<T> >*, const E& )

template<typename T, typename E, Level l>
	OPERATION(Insert, FSObject<l,std::set<T> >* ro, const E& t){

	if (FSDir<l,T>* dir = dynamic_cast<FSDir<l,T>*>(ro)) {
		FSObject<l,T> obj(dir->filename + std::to_string(gensym()),t);
		return true;
	}

	assert(false && "didn't pass me an FSDIR!");
	return false;

}
END_OPERATION
