#include "launch_test.hpp"
#include <chrono>
#include "GoogleGroups.hpp"


using namespace myria;
using namespace mtl;
using namespace std;
using namespace chrono;
using namespace mutils;
using namespace tracker;

template<typename T>
struct causal_newobject{
    TrackerMem &grm;
    newObject_f<Handle<Level::causal,HandleAccess::all,T> > newobj_f(std::unique_ptr<VMObjectLog> &log, int) {
        auto &grm = this->grm;
        return [&grm](const T& t){
            auto trans = start_transaction(log,
                                           grm.trk,
                                           grm.ss.inst(get_strong_ip()),
                                           grm.sc.inst(0));
            trans->commit_on_delete = true;
            return grm.sc.inst(0).
                    template newObject<HandleAccess::all>(grm.trk, trans.get(),t);
        };
    }
};

template<typename T>
struct causal_newset{
    TrackerMem &grm;
    newObject_f<typename remote_set::p<Level::causal,T> > newobj_f(std::unique_ptr<VMObjectLog> &log, int) {
        auto &grm = this->grm;
        return [&grm](const std::set<T>& t){
            auto trans = start_transaction(log,
                                           grm.trk,
                                           grm.ss.inst(get_strong_ip()),
                                           grm.sc.inst(0));
            trans->commit_on_delete = true;
            return grm.sc.inst(0).
                    template newObject<HandleAccess::all>(grm.trk, trans.get(),t);
        };
    }
};

template<typename T>
struct strong_newobject{
    TrackerMem &grm;
    newObject_f<Handle<Level::strong,HandleAccess::all,T> > newobj_f(std::unique_ptr<VMObjectLog> &log, int) {
        auto &grm = this->grm;
        return [&grm](const T& t){
            auto trans = start_transaction(log,
                                           grm.trk,
                                           grm.ss.inst(get_strong_ip()),
                                           grm.sc.inst(0));
            trans->commit_on_delete = true;
            return grm.ss.inst(get_strong_ip()).
                    template newObject<HandleAccess::all>(grm.trk, trans.get(),t);
        };
    }
};

struct TestParameters{
        using time_t = std::chrono::time_point<std::chrono::high_resolution_clock>;
        const time_t start_time{high_resolution_clock::now()};
        time_t last_rate_raise{start_time};
        Frequency current_rate{1000_Hz};
        constexpr static Frequency increase_factor = 20_Hz;
        constexpr static seconds increase_delay = 2s;
        constexpr static minutes test_stop_time = 7min;
        constexpr static double percent_writes = write_percent;
        constexpr static double percent_strong = strong_percent;

	constexpr static Name min_name = 15;
	constexpr static Name max_name = 400000;
	constexpr static double room_name_param = 0.5;

	struct GroupRemember{
		
        TrackerMem transaction_metadata;
		const std::size_t username;
		
        GroupRemember(int id)
            :transaction_metadata(id),username(id % (max_name - min_name) + min_name){}
	};

	struct TestArguments{
        microseconds start_time;
        std::size_t rooms_index;
	};

static_assert(std::is_pod<TestArguments>::value, "Error: need POD for serialization");
	
        using PreparedTest = PreparedTest<GroupRemember,TestArguments>;
        using Pool = typename PreparedTest::Pool;

        //ids between 15 and 400000 should work (as long as they're not ints)

	
        static std::vector<user> users; //these are immutable once initialized
        static std::vector<room> rooms; //these are immutable once initialized
	
	static std::size_t get_room_name() {
		auto ret = get_zipfian_value(max_name - min_name,room_name_param);
		if (ret > (max_name - min_name)) {
			std::cerr << "Name out of range! Trying again" << std::endl;
			return get_room_name(room_name_param);
		}
		else return ret + min_name;
	}

/*	static std::size_t get_user_name(){
		return (int_rand() % (max_name - min_name)) + min_name;
		}/*/

        pair<int,TestArguments> choose_action(Pool&) const {
                bool do_write = better_rand() < percent_writes;
                bool is_strong = (better_rand() < percent_strong || !causal_enabled) && strong_enabled;

				
				TestArguments ta;
				ta.rooms_index = get_room_name();
				ta.start_time = elapsed_time();
				
                //join group
                if (do_write && is_strong) return pair<int,TestArguments>(1,ta);
                //post message
                else if (do_write && !is_strong) return pair<int,TestArguments>(0,ta);
                //check messages
                else return pair<int,TestArguments>(2,ta);
        }

        bool stop (Pool&) const {
                return (high_resolution_clock::now() - start_time) >= test_stop_time;
        };

        milliseconds delay(Pool& p){
                if (high_resolution_clock::now() - last_rate_raise > increase_delay){
                        current_rate += increase_factor;
                        last_rate_raise = high_resolution_clock::now();
                }
                //there should always be 10 request/second/client
                p.set_mem_to(current_rate.hertz / 10);
                return getArrivalInterval(current_rate);
        }

#define method_to_fun(foo,Arg) [](auto& x, Arg y){return x.foo(y);}
        std::string run_tests(PreparedTest& launcher){
                bool (*stop) (TestParameters&,Pool&) = method_to_fun(stop,Pool&);
                pair<int,fake_time> (*choose) (TestParameters&,Pool&) = method_to_fun(choose_action,Pool&);
                milliseconds (*delay) (TestParameters&,Pool&) = method_to_fun(delay,Pool&);
                auto ret = launcher.run_tests(*this,stop,choose,delay);

                global_log.addField(GlobalsFields::request_frequency_final,current_rate);
                return ret;
        }

        abs_StructBuilder &global_log;

        TestParameters(decltype(global_log) &gl):global_log(gl){
                global_log.addField(GlobalsFields::request_frequency,current_rate);
                global_log.addField(GlobalsFields::request_frequency_step,increase_factor);
        }
};
constexpr Frequency TestParameters::increase_factor;
constexpr seconds TestParameters::increase_delay;
constexpr minutes TestParameters::test_stop_time;
constexpr double TestParameters::percent_writes;
constexpr double TestParameters::percent_strong;
constexpr Name TestParameters::min_name;
constexpr Name TestParameters::max_name;

int main(){
	ofstream logFile;
	logFile.open(log_name);
	//auto prof = VMProfiler::startProfiling();
	//auto longpause = prof->pause();
	auto logger = build_VMObjectLogger();
	auto global_log = logger->template beginStruct<LoggedStructs::globals>();
	std::cout << "hello world from VM "<< my_unique_id << " in group " << CAUSAL_GROUP << std::endl;
	std::cout << "connecting to " << string_of_ip(get_strong_ip()) << std::endl;

	using pool_fun_t = typename TestParameters::PreparedTest::action_t;
	
	const std::vector<typename TestParameters::Pool::action_fp> actions{{
                //post message
		[](int tid, shared_ptr<SQLMem> smem, int uid, std::shared_ptr<typename TestParameters::GroupRemember> gmem, int tid, TestArguments args){
                        auto log = gmem->
                                transaction_metadata.log_builder->
                                template beginStruct<LoggedStructs::log>();
                        log->addField(LogFields::submit_time,duration_cast<milliseconds>(args.start_time).count());
                        log->addField(LogFields::run_time,duraction_cast<milliseconds>(elapsed_time()).count());

						TestParameters::rooms.at(args.rooms_index).
                                add_post(log,gmem->transaction_metadata.trk,
                                         post::mke(causal_newobject<post>{gmem->transaction_metadata}.newobj_f(log,tid),"test message"));
                        log->addField(LogFields::done_time,duration_cast<milliseconds>(elapsed_time()).count());
                        return log->single();
                },
        //join group
        [](int tid, shared_ptr<SQLMem> smem, int uid, std::shared_ptr<typename TestParameters::GroupRemember> gmem, int tid, TestArguments args){
                auto log = gmem->
                        transaction_metadata.log_builder->
                        template beginStruct<LoggedStructs::log>();
                log->addField(LogFields::submit_time,duration_cast<milliseconds>(args.start_time).count());
                log->addField(LogFields::run_time,duraction_cast<milliseconds>(elapsed_time()).count());
				TestParameters::rooms().at(args.rooms_index).add_member(log,gmem->transaction_metadata.trk,TestParameters::users().at(gmem->username));
                log->addField(LogFields::done_time,duration_cast<milliseconds>(elapsed_time()).count());
                return log->single();
        },
			//read inbox
			[](int tid, shared_ptr<SQLMem> smem, int uid, std::shared_ptr<typename TestParameters::GroupRemember> gmem, int tid, TestArguments args){
                auto log = gmem->
					transaction_metadata.log_builder->
					template beginStruct<LoggedStructs::log>();
                log->addField(LogFields::submit_time,duration_cast<milliseconds>(args.start_time).count());
                log->addField(LogFields::run_time,duraction_cast<milliseconds>(elapsed_time()).count());
				user::posts_p(log,gmem->transaction_metadata.trk,TestParameters::users().at(gmem->username));
				TestParameters::rooms().at(args.rooms_index).add_member(log,gmem->transaction_metadata.trk,TestParameters::users().at(gmem->username));
                log->addField(LogFields::done_time,duration_cast<milliseconds>(elapsed_time()).count());
                return log->single();
        }

        }};
	
	typename TestParameters::PreparedTest launcher{
		num_processes,vec};
	
	std::cout << "beginning subtask generation loop" << std::endl;

	synth_test::TestParameters params{*global_log};
	const std::string results = params.run_tests(launcher);
		
	//resume profiling
	
	global_log->addField(GlobalsFields::final_completion_time,
						 duration_cast<milliseconds>(elapsed_time()).count());

	logFile << logger->declarations() << endl;

	logFile << global_log->single() << endl;
	logFile << results;
}
