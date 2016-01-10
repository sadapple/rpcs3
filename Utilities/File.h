#pragma once

#include "SharedMutex.h"

namespace fom // file open mode
{
	enum open_mode : u32
	{
		read = 1 << 0, // enable reading
		write = 1 << 1, // enable writing
		append = 1 << 2, // enable appending (always write to the end of file)
		create = 1 << 3, // create file if it doesn't exist
		trunc = 1 << 4, // clear opened file if it's not empty
		excl = 1 << 5, // failure if the file already exists (used with `create`)

		rewrite = write | create | trunc,
	};
};

namespace fs
{
	// File seek mode
	enum seek_mode : u32
	{
		seek_set,
		seek_cur,
		seek_end,
	};

	// File attributes (TODO)
	struct stat_t
	{
		bool is_directory;
		bool is_writable;
		u64 size;
		s64 atime;
		s64 mtime;
		s64 ctime;
	};

	// File handle base
	struct file_base
	{
		virtual ~file_base() = default;

		virtual stat_t stat() = 0;
		virtual bool trunc(u64 length) = 0;
		virtual u64 read(void* buffer, u64 size) = 0;
		virtual u64 write(const void* buffer, u64 size) = 0;
		virtual u64 seek(s64 offset, seek_mode whence) = 0;
		virtual u64 size() = 0;
	};

	// Directory entry (TODO)
	struct dir_entry : stat_t
	{
		std::string name;
	};

	// Directory handle base
	struct dir_base
	{
		virtual ~dir_base() = default;

		virtual bool read(dir_entry&) = 0;
		virtual void rewind() = 0;
	};

	// Virtual device
	struct device_base
	{
		virtual ~device_base() = default;

		virtual bool stat(const std::string& path, stat_t& info) = 0;
		virtual bool remove_dir(const std::string& path) = 0;
		virtual bool create_dir(const std::string& path) = 0;
		virtual bool rename(const std::string& from, const std::string& to) = 0;
		virtual bool remove(const std::string& path) = 0;
		virtual bool trunc(const std::string& path, u64 length) = 0;

		virtual std::unique_ptr<file_base> open(const std::string& path, u32 mode) = 0;
		virtual std::unique_ptr<dir_base> open_dir(const std::string& path) = 0;
	};

	class device_manager final
	{
		mutable shared_mutex m_mutex;

		std::unordered_map<std::string, std::shared_ptr<device_base>> m_map;

	public:
		std::shared_ptr<device_base> get_device(const std::string& name);
		std::shared_ptr<device_base> set_device(const std::string& name, const std::shared_ptr<device_base>&);
	};

	// Get virtual device for specified path (nullptr for real path)
	std::shared_ptr<device_base> get_virtual_device(const std::string& path);

	// Set virtual device with specified name (nullptr for deletion)
	std::shared_ptr<device_base> set_virtual_device(const std::string& root_name, const std::shared_ptr<device_base>&);

	// Get parent directory for the path (returns empty string on failure)
	std::string get_parent_dir(const std::string& path);

	// Get file information
	bool stat(const std::string& path, stat_t& info);

	// Check whether a file or a directory exists (not recommended, use is_file() or is_dir() instead)
	bool exists(const std::string& path);

	// Check whether the file exists and is NOT a directory
	bool is_file(const std::string& path);

	// Check whether the directory exists and is NOT a file
	bool is_dir(const std::string& path);

	// Delete empty directory
	bool remove_dir(const std::string& path);

	// Create directory
	bool create_dir(const std::string& path);

	// Create directories
	bool create_path(const std::string& path);

	// Rename (move) file or directory
	bool rename(const std::string& from, const std::string& to);

	// Copy file contents
	bool copy_file(const std::string& from, const std::string& to, bool overwrite);

	// Delete file
	bool remove_file(const std::string& path);

	// Change file size (possibly appending zeros)
	bool truncate_file(const std::string& path, u64 length);

	class file final
	{
		std::unique_ptr<file_base> m_file;

	public:
		file() = default;

		// Open file handler
		explicit file(const std::string& path, u32 mode = fom::read)
		{
			open(path, mode);
		}

		// Import file handler
		template<typename T, typename = std::enable_if_t<std::is_base_of<file_base, T>::value>>
		file(std::unique_ptr<T>&& _file)
			: m_file(std::move(_file))
		{
		}

		// Forbid nullptr arg
		file(std::nullptr_t) = delete;

		// Check whether the handle is valid (opened file)
		explicit operator bool() const
		{
			return m_file.operator bool();
		}

		// Open specified file with specified mode
		bool open(const std::string& path, u32 mode = fom::read);

		// Close the file explicitly, return file handler
		std::unique_ptr<file_base> release()
		{
			return std::move(m_file);
		}

		// Change file size (possibly appending zero bytes)
		bool trunc(u64 length) const
		{
			Expects(m_file);
			return m_file->trunc(length);
		}

		// Get file information
		stat_t stat() const
		{
			Expects(m_file);
			return m_file->stat();
		}

		// Read the data from the file and return the amount of data written in buffer
		u64 read(void* buffer, u64 count) const
		{
			Expects(m_file);
			return m_file->read(buffer, count);
		}

		// Write the data to the file and return the amount of data actually written
		u64 write(const void* buffer, u64 count) const
		{
			Expects(m_file);
			return m_file->write(buffer, count);
		}

		// Change current position, returns previous position
		u64 seek(s64 offset, seek_mode whence = seek_set) const
		{
			Expects(m_file);
			return m_file->seek(offset, whence);
		}

		// Get file size
		u64 size() const
		{
			Expects(m_file);
			return m_file->size();
		}

		// Get current position
		u64 pos() const
		{
			Expects(m_file);
			return m_file->seek(0, seek_cur);
		}

		// Write std::string unconditionally
		const file& write(const std::string& str) const
		{
			ASSERT(write(str.data(), str.size()) == str.size());
			return *this;
		}

		// Write POD unconditionally
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, const file&> write(const T& data) const
		{
			ASSERT(write(std::addressof(data), sizeof(T)) == sizeof(T));
			return *this;
		}

		// Write POD std::vector unconditionally
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, const file&> write(const std::vector<T>& vec) const
		{
			ASSERT(write(vec.data(), vec.size() * sizeof(T)) == vec.size() * sizeof(T));
			return *this;
		}

		// Read std::string, size must be set by resize() method
		bool read(std::string& str) const
		{
			return read(&str[0], str.size()) == str.size();
		}

		// Read POD, sizeof(T) is used
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, bool> read(T& data) const
		{
			return read(&data, sizeof(T)) == sizeof(T);
		}

		// Read POD std::vector, size must be set by resize() method
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, bool> read(std::vector<T>& vec) const
		{
			return read(vec.data(), sizeof(T) * vec.size()) == sizeof(T) * vec.size();
		}

		// Read POD (experimental)
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, T> read() const
		{
			T result;
			ASSERT(read(result));
			return result;
		}

		// Read full file to std::string
		std::string to_string() const
		{
			std::string result;
			result.resize(size());
			ASSERT(seek(0) == 0 && read(result));
			return result;
		}

		// Read full file to std::vector
		template<typename T>
		std::enable_if_t<std::is_pod<T>::value && !std::is_pointer<T>::value, std::vector<T>> to_vector() const
		{
			std::vector<T> result;
			result.resize(size() / sizeof(T));
			ASSERT(seek(0) == 0 && read(result));
			return result;
		}
	};

	class dir final
	{
		std::unique_ptr<dir_base> m_dir;

	public:
		dir() = default;

		// Open dir handle
		explicit dir(const std::string& path)
		{
			open(path);
		}

		// Import dir handler
		template<typename T, typename = std::enable_if_t<std::is_base_of<dir_base, T>::value>>
		dir(std::unique_ptr<T>&& _dir)
			: m_dir(std::move(_dir))
		{
		}

		// Forbid nullptr arg
		dir(std::nullptr_t) = delete;

		// Check whether the handle is valid (opened directory)
		explicit operator bool() const
		{
			return m_dir.operator bool();
		}

		// Open specified directory
		bool open(const std::string& path);
		
		// Close the directory explicitly, return dir_base
		std::unique_ptr<dir_base> release()
		{
			return std::move(m_dir);
		}

		// Get next directory entry
		bool read(dir_entry& out)
		{
			Expects(m_dir);
			return m_dir->read(out);
		}

		// Reset to the beginning
		void rewind()
		{
			Expects(m_dir);
			return m_dir->rewind();
		}

		class iterator
		{
			dir* m_parent;
			dir_entry m_entry;

		public:
			enum class mode
			{
				from_first,
				from_current
			};

			iterator(dir* parent, mode mode_ = mode::from_first)
				: m_parent(parent)
			{
				if (!m_parent)
				{
					return;
				}

				if (mode_ == mode::from_first)
				{
					m_parent->rewind();
				}

				if (!m_parent->read(m_entry))
				{
					m_parent = nullptr;
				}
			}

			dir_entry& operator *()
			{
				return m_entry;
			}

			iterator& operator++()
			{
				*this = { m_parent, mode::from_current };
				return *this;
			}

			bool operator !=(const iterator& rhs) const
			{
				return m_parent != rhs.m_parent;
			}
		};

		iterator begin()
		{
			return{ m_dir ? this : nullptr };
		}

		iterator end()
		{
			return{ nullptr };
		}
	};

	// Get configuration directory
	const std::string& get_config_dir();

	// Get executable directory
	const std::string& get_executable_dir();

	// Delete directory and all its contents recursively
	void remove_all(const std::string& path);

	// Get size of all files recursively
	u64 get_dir_size(const std::string& path);
}

// For compatibility (it's not a good idea)
class memory_stream final : public fs::file_base
{
	u64 m_pos;
	u64 m_size;

public:
	const std::add_pointer_t<char> ptr;

	[[deprecated]] memory_stream(void* ptr, u64 max_size)
		: m_pos(0)
		, m_size(max_size)
		, ptr(static_cast<char*>(ptr))
	{
	}

	fs::stat_t stat() override
	{
		throw std::logic_error("memory_stream doesn't support stat()");
	}

	bool trunc(u64 length) override
	{
		throw std::logic_error("memory_stream doesn't support trunc()");
	}

	u64 read(void* buffer, u64 count) override
	{
		const u64 start = m_pos;
		const u64 end = seek(count, fs::seek_cur);
		const u64 read_size = end >= start ? end - start : throw std::logic_error("memory_stream::read(): overflow");
		std::memmove(buffer, ptr + start, read_size);
		return read_size;
	}

	u64 write(const void* buffer, u64 count) override
	{
		const u64 start = m_pos;
		const u64 end = seek(count, fs::seek_cur);
		const u64 write_size = end >= start ? end - start : throw std::logic_error("memory_stream::write(): overflow");
		std::memmove(ptr + start, buffer, write_size);
		return write_size;
	}

	u64 seek(s64 offset, fs::seek_mode whence) override
	{
		return m_pos =
			whence == fs::seek_set ? std::min<u64>(offset, m_size) :
			whence == fs::seek_cur ? std::min<u64>(offset + m_pos, m_size) :
			whence == fs::seek_end ? std::min<u64>(offset + m_size, m_size) :
			throw std::logic_error("memory_stream::seek(): invalid whence");
	}

	u64 size() override
	{
		return m_size;
	}
};
