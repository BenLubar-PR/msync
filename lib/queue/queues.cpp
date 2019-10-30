#include "queues.hpp"

#include <constants.hpp>
#include <filesystem.hpp>
#include <system_error>
#include <print_logger.hpp>
#include "../options/global_options.hpp"
#include "../postfile/outgoing_post.hpp"
#include <algorithm>
#include <msync_exception.hpp>


fs::path get_file_queue_directory(std::string_view account)
{
	return options().executable_location / Account_Directory / account / File_Queue_Directory;
}

void unique_file_name(fs::path& path)
{
	unsigned int extensionint = 1;
	path += ".0";
	do
	{
		path.replace_extension(std::to_string(extensionint++));
	} while (fs::exists(path));
}

void queue_attachments(const fs::path& postfile)
{
	outgoing_post post{ postfile };
	std::error_code err;
	for (auto& attach : post.parsed.attachments)
	{
		const auto attachpath = fs::canonical(attach, err);

		if (err)
		{
			pl() << "Error finding file: " << attach << "\nError:\n" << err << "\nSkipping.\n";
			continue;
		}

		if (!fs::is_regular_file(attachpath))
		{
			pl() << attachpath << " is not a regular file. Skipping.\n";
			continue;
		}

		attach = attachpath.string();

		err.clear();
	}
}

std::string queue_post(const fs::path& queuedir, const fs::path& postfile)
{
	fs::create_directories(queuedir);

	if (!fs::exists(postfile))
	{
		pl() << "Could not find " << postfile << ". Skipping.\n";
		return "";
	}

	if (!fs::is_regular_file(postfile))
	{
		pl() << postfile << " is not a file. Skipping.\n";
		return "";
	}

	fs::path copyto = queuedir / postfile.filename();

	if (fs::exists(copyto))
	{
		pl() << copyto << " already exists. " << postfile << " will be saved as ";
		unique_file_name(copyto);
		pl() << copyto << '\n';
	}

	fs::copy(postfile, copyto);

	queue_attachments(copyto);

	return copyto.filename().string();
}

queue_list open_queue(const queues to_open, const std::string_view account)
{
	fs::path qfile = options().executable_location / Account_Directory / account;
	fs::create_directories(qfile);
	const std::string_view to_append = [to_open]() {
		switch (to_open)
		{
		case queues::fav:
			return Fav_Queue_Filename;
			break;
		case queues::boost:
			return Boost_Queue_Filename;
			break;
		case queues::post:
			return Post_Queue_Filename;
			break;
		default:
			throw msync_exception("whoops, this shouldn't happen.");
		}
	}();

	return queue_list{ qfile / to_append };
}

void enqueue(const queues toenqueue, const std::string_view account, const std::vector<std::string>& add)
{
	queue_list toaddto = open_queue(toenqueue, account);

	const fs::path filequeuedir = toenqueue == queues::post ? get_file_queue_directory(account) : "";
	for (const auto& id : add)
	{
		std::string queuethis = id;
		if (toenqueue == queues::post)
		{
			queuethis = queue_post(filequeuedir, id);
		}
		toaddto.parsed.push_back(std::move(queuethis));
	}

	// consider looking for those "delete" guys, the ones with the - at the end, and having this cancel them out, 
	// but "unboost and reboost", for example, is a valid thing to want to do.
}

void dequeue_post(const fs::path &queuedir, const fs::path& filename)
{
	if (!fs::remove(queuedir / filename))
	{
		pl() << "Could not delete " << filename << ", could not find it in " << queuedir << '\n';
	}
}

void just_filename(std::string& path)
{
	path = fs::path(path).filename().string();
}

void dequeue(queues todequeue, const std::string_view account, std::vector<std::string>&& toremove)
{
	queue_list toremovefrom = open_queue(todequeue, account);

	const fs::path filequeuedir = todequeue == queues::post ? get_file_queue_directory(account) : "";
	for (auto it = toremove.begin(); it != toremove.end(); )
	{
		if (todequeue == queues::post)
			just_filename(*it);
		
		// if the item is in the queue, remove it form the queue and from the toremove vector
		// if this gets to be a performance bottleneck (since it's an n^2 algorithm), make an unordered_set from toremove or sort and do a binary search
		const auto inqueue = std::find(toremovefrom.parsed.begin(), toremovefrom.parsed.end(), *it);
		if (inqueue != toremovefrom.parsed.end()) // if we found the thing in the queue, remove from both
		{
			plverb() << "Removing " << *it << " from queue.\n";
			if (todequeue == queues::post)
			{
				dequeue_post(filequeuedir, *it);
			}
			toremovefrom.parsed.erase(inqueue);
			it = toremove.erase(it);
		}
		else
		{
			++it;
		}
	}

	//basically, if a thing isn't in the queue, enqueue removing that thing. unboosting, unfaving, deleting a post
	//consider removing duplicate removes?

	for (auto& queuedel : toremove)
	{
		plverb() << queuedel << " will be removed next time you sync.\n";
		queuedel.push_back('-');
		toremovefrom.parsed.push_back(std::move(queuedel));
	}
}

void clear(queues toclear, const std::string_view account)
{
	queue_list clearthis = open_queue(toclear, account);

	clearthis.parsed.clear();

	if (toclear == queues::post)
	{
		fs::remove_all(get_file_queue_directory(account));
	}
}

queue_list get(queues toget, const std::string_view account)
{
	return open_queue(toget, account);
}

std::vector<std::string> print(queues toprint, const std::string_view account)
{
	//prettyprint posts
	const queue_list printthis = open_queue(toprint, account);
	return std::vector<std::string> {printthis.parsed.begin(), printthis.parsed.end()};
}
