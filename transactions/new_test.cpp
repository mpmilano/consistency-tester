#include "IfBuilder.hpp"
#include "TransactionBuilder.hpp"
#include "Transaction.hpp"
#include "BaseCS.hpp"
#include "FileStore.hpp"
#include "Operate.hpp"
#include "TempBuilder.hpp"
#include "FreeExpr.hpp"
#include "Transaction_macros.hpp"

int main(){
	try {

		{

			//FileStore<Level::causal> fsc;
			FileStore<Level::strong> fs;

			auto num_dir = fs.newCollection<HandleAccess::all, int>();
			{
				std::set<int> test;
				test.insert(13);
				num_dir.put(test);
				num_dir.get(); //just to see if it'll crash
			}

			TRANSACTION(
				let_mutable(tmp) = true IN (
					IF (tmp) THEN(
						do_op(Insert,num_dir,42)
						),
					tmp = free_expr(bool,num_dir, num_dir.empty());,
					WHILE (!tmp) DO (dummy1, tmp = false),
					IF (isValid(num_dir)) THEN (dummy1;)
					)
				);
		}

		{

			FileStore<Level::causal> fs;

			auto num_dir = fs.newCollection<HandleAccess::all, int>();
			{
				std::set<int> test;
				test.insert(13);
				num_dir.put(test);
				num_dir.get(); //just to see if it'll crash
			}

			TRANSACTION(
				let_mutable(tmp) = true IN (
					IF (tmp) THEN(
						do_op(Insert,num_dir,42)
						),
					tmp = free_expr(bool,num_dir, num_dir.empty());,
					WHILE (!tmp) DO (dummy2;),
					IF (isValid(num_dir)) THEN (dummy2;)
					)
				);
		}

		//*/
	}
	catch (NoOverloadFoundError e){
		std::cerr << "No overload found: " << e.msg << std::endl;
	}

}
