#pragma once

#include "Operation.hpp"
#include "DataStore.hpp"
#include "Transaction.hpp"
#include <memory>
#include <vector>
#include <array>

/**
   Information: We are assuming an SQL store which has already been configured
   under the following assumptions:

   (1) There exists a table, BlobStore, with columns (id, data), in which we
   can just throw a binary blob.  This is used for objects which the store
   does not know how to handle.

*/

namespace myria { namespace pgsql {

		template<Level l>
		class SQLStore;

		enum class Table{
			BlobStore = 0,IntStore = 1
				};

		static constexpr int Table_max = 2;

		const std::string& table_name(Table t);

		struct SQLStore_impl {
		private:
	
			SQLStore_impl(GDataStore &store, int instanceID, Level);
			GDataStore &_store;
		public:
			virtual ~SQLStore_impl();

			template<Level l>
			friend class SQLStore;

			struct SQLConnection;
			using SQLConnection_t = SQLConnection*;
			std::array<int, NUM_CAUSAL_GROUPS> clock;

			const Level level;
			SQLConnection_t default_connection;
	
			SQLStore_impl(const SQLStore_impl&) = delete;
	
			std::unique_ptr<mtl::TransactionContext> begin_transaction();
	
			int instance_id() const;
			bool exists(Name id);
			void remove(Name id);

			int ds_id() const;
			static constexpr int ds_id_nl(){ return 2;}
	
			struct GSQLObject {
				struct Internals;
			private:
				Internals *i;
				GSQLObject(Name id, int size);
			public:
				GSQLObject(SQLStore_impl &ss, Table t, Name name, const std::vector<char> &c);
				GSQLObject(SQLStore_impl &ss, Table t, Name name, int size);
				GSQLObject(SQLStore_impl &ss, Table t, Name name);
				GSQLObject(const GSQLObject&) = delete;
				GSQLObject(GSQLObject&&);
				void save();
				char* load();
				char* obj_buffer();
				char const * const obj_buffer() const ;
				int obj_buffer_size() const;
				const std::array<int,NUM_CAUSAL_GROUPS>& timestamp() const ;
				SQLStore_impl& store();

				//will crash if stored object is non-integral.
				void increment();

				//required by GeneralRemoteObject
				void setTransactionContext(mtl::TransactionContext*);
				mtl::TransactionContext* currentTransactionContext();
				bool ro_isValid() const;
				int store_instance_id() const;
				Name name() const;

				//required by ByteRepresentable
				int bytes_size() const;
				int to_bytes(char*) const;
				static GSQLObject from_bytes(char* v);
				virtual ~GSQLObject();
			};

			//operations

		};

	}}