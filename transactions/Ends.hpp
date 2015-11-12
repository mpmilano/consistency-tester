#pragma once
#include <algorithm>

namespace{
	namespace ends{
		std::array<int,4> max(const std::array<int,4> &a,const std::array<int,4> &b){
			return std::array<int,4> {{std::max(a[0],b[0]),std::max(a[1],b[1]),std::max(a[2],b[2]),std::max(a[3],b[3])}};
		}
		
		template<size_t s>
		bool is_same(const std::array<int,s> &a,const std::array<int,s> &b){
			for (int i = 0; i < s; ++i){
				if (a[i] != b[i]) return false;
			}
			return true;
		}
		
		template<size_t s>
		bool prec(const std::array<int,s> &a,const std::array<int,s> &b){
			for (int i = 0; i < s; ++i){
				assert(a[i] != -1);
				assert(b[i] != -1);
				if (a[i] > b[i]) return false;
			}
			return true;
		}
	}
}
