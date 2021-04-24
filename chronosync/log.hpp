namespace logging = boost::log;
namespace keywords = boost::log::keywords;

void initlogger(std::string filename) {
	logging::add_common_attributes();
	logging::add_file_log
	(
		keywords::file_name = filename,
		keywords::format = "\"%TimeStamp%\", \"%ProcessID%\", \"%ThreadID%\", \"%Message%\"",
		keywords::open_mode = std::ios_base::app,
		keywords::auto_flush = true
	);
}

