#pragma once
#include "Transaction.hpp"

template<typename T>
struct CachedObject : public RemoteObject<T> {
	std::unique_ptr<T> t;
	GDataStore &st;
	std::string nm;
	CachedObject(decltype(t) t, GDataStore &st, const std::string &name)
		:t(std::move(t)),st(st),nm(name){}
	
	TransactionContext* currentTransactionContext(){
		assert(false && "you probably didn't mean to call this");
	}
	
	void setTransactionContext(TransactionContext* tc){
		assert(false && "you probably didn't mean to call this");
	}	
	
	const T& get() {
		return *t;
	}
	
	void put(const T&) {
			assert(false && "error: modifying strong Handle in causal context! the type system is supposed to prevent this!");
	}
	
	bool ro_isValid() const {
		return t.get();
	}
	
	const GDataStore& store() const {
		return st;
	}

	const std::string& name() const {
		return nm;
	}
	
	GDataStore& store() {
		return st;
	}
	
	int bytes_size() const {
		assert(false && "wait why are you ... stop!");
	}
	
	int to_bytes(char* v) const {
		assert(false && "wait why are you ... stop!");
	}
		
};

template<HandleAccess ha, typename T>
Handle<Level::strong,ha,T> run_ast_strong(const StrongCache& c, const StrongStore&, const Handle<Level::strong,ha,T>& _h) {

	auto ctx = context::current_context(c);
	auto h = _h.clone();

	assert(ctx != context::t::unknown);

	if (ctx == context::t::read){
		return 
			make_handle<Level::strong,ha,T,CachedObject<T> >
			(heap_copy(h.get()),h.store(),h.name());
	}
	else return h;
}

template<typename T>
	struct LocalObject : public RemoteObject<T> {
		RemoteObject<T>& r;
		LocalObject(decltype(r) t)
			:r(t){}
		
		TransactionContext* currentTransactionContext(){
			assert(false && "you probably didn't mean to call this");
		}
		
		void setTransactionContext(TransactionContext* tc){
			assert(false && "you probably didn't mean to call this");
		}	
		
		const T& get() {
			//if the assert passes, then this was already fetched.
			//if the assert fails, then either we shouldn't have gone this deep
			//with causal calls,
			//or we have an info-flow violation,
			//or we can't infer the right level for operations (which would be weird).

			if(auto *co = dynamic_cast<CachedObject<T>* >(&r)){
				return co->get();
			}
			else assert(false && "Error: attempt get on non CachedObject. This should have been cached earlier");
		}
		
		void put(const T&) {
			assert(false && "error: modifying strong Handle in causal context! the type system is supposed to prevent this!");
		}
		
		bool ro_isValid() const {
			if(auto *co = dynamic_cast<CachedObject<T>* >(&r)){
				return co->ro_isValid();
			}
			else assert(false && "Error: attempt isValid on non CachedObject. This should have been cached earlier");
		}
		
		const GDataStore& store() const {
			return r.store();
		}
		
		const std::string& name() const {
			return r.name();
		}
		
		GDataStore& store() {
			return r.store();
		}
		
		int bytes_size() const {
			assert(false && "wait why are you ... stop!");
		}
		
		int to_bytes(char* v) const {
			assert(false && "wait why are you ... stop!");
		}
		
	};	

template<HandleAccess ha, typename T>
Handle<Level::strong,ha,T> run_ast_causal(CausalCache& cache, const CausalStore &s, const Handle<Level::strong,ha,T>& h) {
	return 
		make_handle<Level::strong,ha,T,LocalObject<T> >
		(h.remote_object());
}
