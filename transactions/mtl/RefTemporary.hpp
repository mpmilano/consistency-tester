#pragma once
#include "TemporaryMutation.hpp"
#include "MutableTemporary.hpp"

namespace myria { namespace mtl {

		template<unsigned long long ID, Level l, typename T, typename Temp>
		struct RefTemporaryCommon : public ConExpr<run_result<T>,l> {
		private:
			RefTemporaryCommon(const Temp& t, const std::string& name, int id):t(t),name(name),id(id){}
		public:
	
			const Temp t;
			const std::string name;

			//Note: this ID will change
			//every time we copy this class.
			//every copy should have a unique ID.
			const int id = mutils::gensym();

			RefTemporaryCommon(const Temp &t):t(t),name(t.name) {}

			RefTemporaryCommon(const RefTemporaryCommon& rt):t(rt.t),name(rt.name){
				assert(!debug_forbid_copy);
			}
	
			RefTemporaryCommon(RefTemporaryCommon&& rt):t(rt.t),name(rt.name),id(rt.id){}

			RefTemporaryCommon clone() const {
				return RefTemporaryCommon(t,name,id);
			}

			auto environment_expressions() const {
				return t.environment_expressions();
			}

			auto strongCall(TransactionContext* ctx, StrongCache& cache, const StrongStore &s) const {
				//TODO - endorsements should happen somewhere around here, right?
				//todo: dID I want the level of the expression which assigned the temporary?
		
				choose_strong<get_level<Temp>::value > choice{nullptr};
				try {
					return strongCall(ctx,cache, s,choice);
				}
				catch (const StoreMiss&){
					std::cerr << "tried to reference variable " << name << std::endl;
					assert(false && "we don't have that in the store");
				}
			}

			auto strongCall(TransactionContext* ctx, StrongCache& cache, const StrongStore &s, std::true_type*) const {
				//std::cout << "inserting RefTemp " << name << " (" << id<< ") into cache "
				//		  << &cache << std::endl;
				auto ret = call<StoreType::StrongStore>(s, t);
				cache.insert(id,ret);
				//std::cout << "RefTemp ID " << this->id << " inserting into Cache " << &cache << " value: " << ret << std::endl;
				return ret;
			}

			void strongCall(TransactionContext* ctx, StrongCache& cache, const StrongStore &s, std::false_type*) const {
				//we haven't even done the assignment yet. nothing to see here.
			}

			auto causalCall(TransactionContext* ctx, const CausalCache& cache, const CausalStore &s) const {
		
				typedef decltype(call<StoreType::CausalStore>(s,t)) R;
				if (cache.contains(this->id)) {
					return cache.get<R>(this->id);
				}
				else {
					try {
						return call<StoreType::CausalStore>(s,t);
					}
					catch (const StoreMiss&){
						std::cerr << "Couldn't find this in the store: " << name << std::endl;
						assert(false && "Not in the store");
					}
				}
			}

		private:
			template<StoreType st, restrict(is_store(st))>
			static auto call(const StoreMap<st> &s, const Temporary<ID,l,T> &t) ->
				run_result<decltype(t.t)>
				{
					typedef run_result<decltype(t.t)> R;
					static_assert(mutils::neg_error_helper<is_ConStatement,R>::value,"Static assert failed");
					return s. template get<R>(t.store_id);
				}
		};

		template<unsigned long long ID, Level l, typename T, typename Temp>
		struct RefTemporary;
		
		template<unsigned long long ID, Level l, typename T>
		struct RefTemporary<ID,l,T,Temporary<ID,l,T > > : public RefTemporaryCommon<ID,l,T,Temporary<ID,l,T > > {
			using super_t = RefTemporaryCommon<ID,l,T,Temporary<ID,l,T > >;
			using Temp = Temporary<ID,l,T >;
		private:
			RefTemporary(const Temp& t, const std::string& name, int id):super_t(t,name,id){}
		public:
			RefTemporary(const Temp &t):super_t(t){}

			RefTemporary(const RefTemporary& rt):super_t(rt){}
	
			template<typename E2>
			auto operator=(const E2 &e) const {
				static_assert(is_ConExpr<std::decay_t<decltype(wrap_constants(e))> >::value,"Error: attempt to assign non-Expr");
				return *this << wrap_constants(e);
			}//*/

		};

		template<unsigned long long ID, Level l, typename T>
		struct RefTemporary<ID,l,T,MutableTemporary<ID,l,T > > : public RefTemporaryCommon<ID,l,T,MutableTemporary<ID,l,T > > {
			using super_t = RefTemporaryCommon<ID,l,T,MutableTemporary<ID,l,T > >;
			using Temp = MutableTemporary<ID,l,T >;
		private:
			RefTemporary(const Temp& t, const std::string& name, int id):super_t(t,name,id){}
		public:
			RefTemporary(const Temp &t):super_t(t){}

			RefTemporary(const RefTemporary& rt):super_t(rt){}
			
			template<typename E>
			auto operator=(const E &e) const {
				static_assert(is_ConExpr<E>::value,"Error: attempt to assign non-Expr");
				auto wrapped = wrap_constants(e);
				TemporaryMutation<decltype(wrapped)> r{this->name,this->t.store_id,wrapped};
				return r;
			}

		};

		template<unsigned long long ID, Level l, typename T, typename Temp>
		auto find_usage(const RefTemporaryCommon<ID,l,T,Temp> &rt){
			return mutils::shared_copy(rt.t);
		}

		template<unsigned long long ID, unsigned long long ID2, Level l, typename T, typename Temp>
		std::enable_if_t<ID != ID2, std::nullptr_t> find_usage(const RefTemporaryCommon<ID2,l,T,Temp> &rt){
			return nullptr;
		}

		template<unsigned long long ID, unsigned long long ID2, Level l, typename T, typename Temp>
		struct contains_temporary<ID, RefTemporary<ID2,l,T,Temp> > : std::integral_constant<bool, ID == ID2> {};

		//TODO: figure out why this needs to be here
		template<Level l, typename T, typename E, unsigned long long id>
		struct is_ConExpr<RefTemporary<id,l,T, E> > : std::true_type {};

		struct nope{
			typedef std::false_type found;
		};

		template<Level l, typename T, typename E, unsigned long long id>
		constexpr bool is_reftemp(const RefTemporaryCommon<id,l,T, E> *){
			return true;
		}

		template<typename T>
		constexpr bool is_reftemp(const T*){
			return false;
		}

		template<typename T>
		struct is_RefTemporary : std::integral_constant<bool,is_reftemp(mutils::mke_p<T>())>::type
		{};

	} }
