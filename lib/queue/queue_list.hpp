#ifndef _QUEUE_LIST_HPP_
#define _QUEUE_LIST_HPP_

#include <deque>
#include <string>

#include "../filebacked/file_backed.hpp"

enum class api_route : uint8_t
{
	fav,
	unfav,
	boost,
	unboost,
	post,
	unpost, // you might call it "delete post"
	unknown,
};

struct api_call
{
	api_route queued_call;
	std::string argument;
};

bool Read(std::deque<api_call>&, std::string&&);
void Write(std::deque<api_call>&&, std::ofstream&);

using queue_list = file_backed<std::deque<api_call>, Read, Write>;
using readonly_queue_list = file_backed<std::deque<api_call>, Read, Write, true, true, true>;
#endif
