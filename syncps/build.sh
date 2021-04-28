g++ eval.cpp -o ../eval -DBOOST_LOG_DYN_LINK --std=c++14 \
                     -lboost_system -lboost_log_setup -lboost_log -lboost_thread \
                     -lboost_iostreams -lpthread -lndn-cxx
