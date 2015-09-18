#include "utils.hpp"

std::nullptr_t choose_non_np(std::nullptr_t, std::nullptr_t){
	return nullptr;
}

std::nullptr_t dref_np(std::nullptr_t*){
	return nullptr;
}

int gensym() {
	static int counter = 0;
	assert(counter < (std::numeric_limits<int>::max() - 1));
	return ++counter;
}

std::vector<std::string> read_dir(const std::string &name){

	std::vector<std::string> ret;
	
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (name.c_str()))) {
		/* print all the files and directories within directory */
		while ((ent = readdir (dir))) {
			std::string maybe(ent->d_name);
			if (maybe == "." || maybe == "..") continue;
			ret.push_back(std::string(maybe));
		}
		closedir (dir);
	} else {
		/* could not open directory */
		perror ("");
		assert(false && "Could not open dir.");
	}

	return ret;
}

void break_here(){}

