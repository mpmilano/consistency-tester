
#include "IfBuilder.hpp"
#include "TransactionBuilder.hpp"
#include "Transaction.hpp"
#include "BaseCS.hpp"
#include "FileStore.hpp"
#include "Operate.hpp"
#include "TempBuilder.hpp"
#include "FreeExpr.hpp"
#include "Transaction_macros.hpp"
#include "FreeExpr_macros.hpp"
#include "Operate_macros.hpp" //*/
#include "SerializationMacros.hpp"
#include "FinalHeader.hpp"

template<typename T>
FINALIZE_OPERATION(Increment, RemoteObject<T>*);


struct WeakCons :
	public ByteRepresentable {
	Handle<Level::causal, HandleAccess::all, int> val;
	Handle<Level::strong, HandleAccess::all, WeakCons> next;
	
	WeakCons(const decltype(val) *val, const decltype(next) *next)
		:val(*val),next(*next){
		delete val; delete next;
	}

	WeakCons(const decltype(val) &val, const decltype(next) &next)
		:val(val),next(next){}

	DEFAULT_SERIALIZATION_SUPPORT(WeakCons,val,next)
	
};

int main() {

	auto &fsc = 
		FileStore<Level::causal>::filestore_instance();
	auto& fss =
		FileStore<Level::strong>::filestore_instance();
	
	Handle<Level::strong, HandleAccess::all, WeakCons> h0;
	WeakCons initial{fsc.newObject<HandleAccess::all,int>(12),h0};
	Handle<Level::strong, HandleAccess::all, WeakCons> h =
		fss.newObject<HandleAccess::all, WeakCons>(initial);
	


	TRANSACTION(
		let_mutable(hd) = h IN (
			WHILE (isValid(hd)) DO(
				let_ifValid(tmp) = hd IN (
					let_ifValid(weak_val) = msg(tmp,val) IN (do_op(Increment,weak_val)),
					hd = msg(tmp,next)
					)
				)
			)
		)
	
	return 0;
}