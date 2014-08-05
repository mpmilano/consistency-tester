#include "Backend.hpp"

namespace backend {

	
	class DataStore::GenericHandle{
	private: 
		virtual bool is_virtual() = 0;
	};
	
	template <typename T>
	class DataStore::TypedHandle : public DataStore::GenericHandle{
	private: 
		virtual bool is_virtual() = 0;
		DataStore::HandleImpl<T> &h_i;
		DataStore::HandleImpl<T> &hi() const {return h_i;}
	public:
		TypedHandle(DataStore::HandleImpl<T> &hi):h_i(hi){}
		friend class DataStore;
		template<Client_Id>
		friend class Client;
	};


	template<Client_Id id, Level L, HandleAccess HA, typename T>
	class DataStore::Handle : public DataStore::TypedHandle<T> {
	private:
		virtual bool is_virtual() {return false;}
		Handle(HandleImpl<T> &hi):TypedHandle<T>(hi){}
	public:
		static constexpr Level level = L;
		static constexpr HandleAccess ha = HA;
		typedef T stored_type;
		friend class DataStore;
		template<Client_Id>
		friend class Client;
	};

	
	template<Client_Id id, Level L, HandleAccess HA, typename T>	
	DataStore::Handle<id,L,HA,T> DataStore::newhandle_internal(std::unique_ptr<T> r) {
		return Handle<id,L,HA,T>
			(HandleImpl<T>::constructAndPlace(*this,std::move(r)));
	}
	
	
	template<Client_Id id, Level L, HandleAccess HA, typename T>
	std::unique_ptr<T> DataStore::del_internal(Handle<id,L,HA,T> &hndl_i){
		auto &hndl = hndl_i.hi(); 
		std::unique_ptr<T> ret = hndl;
		assert(hndls[hndl.id]->id == hndl.id);
		hndls[hndl.id].first.reset(nullptr);
		return ret;
	}

	
	template<Client_Id cid, Level L, typename T, Level _L, HandleAccess _ha, Client_Id _cid>
	auto DataStore::get_handle(const DataStore::Handle<_cid,_L,_ha,T> &_underlying){
		const auto &underlying = _underlying.hi();
		auto &local = *this;
		auto &master = underlying.parent;
		assert(&local != &master);
		assert(local.hndls[underlying.id].get() == nullptr );
		assert(master.hndls[underlying.id].get() != nullptr );
		assert(master.hndls[underlying.id]->rid == underlying.rid);
		auto &&ret = DataStore::Handle<cid,L,HandleAccess::all,T>(underlying.clone(local));
		return std::move(ret);
	}


	void DataStore::syncClient(DataStore& to) const {
		ReadLock(const_cast<DataStore*>(this)->mut);
		WriteLock(to.mut);
		DataStore const &from = *this;
		for (auto& ptr_p : to.hndls) {
			auto &ptr = ptr_p.second;
			auto const &m_ptr = from.hndls.at(ptr->id);
			if (m_ptr->rid == ptr->rid) {
				ptr->grab_obj(*m_ptr);
			}
		}
	}

}

