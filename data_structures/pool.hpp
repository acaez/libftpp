#ifndef POOL_HPP
# define POOL_HPP

# include <cstddef>
# include <cstdlib>
# include <iostream>
# include <new>
# include <stdexcept>
# include <utility>
# include <vector>

// ============================================================================================= //

template <typename TType>
class Pool
{
	private:
		static_assert(alignof(TType) <= alignof(std::max_align_t),
			"Pool: over-aligned types are not supported");

		union Slot
		{
			Slot							*next;
			alignas(TType) unsigned char	storage[sizeof(TType)];
		};

		std::vector<Slot *>	blocks;
		Slot				*freeList;
		std::size_t			usedCount;

	public:
		static const std::size_t	slt = 32;

		class Object
		{
			private:
				Pool<TType>	*pool;
				TType		*ptr;

				friend class Pool<TType>;

				Object(Pool<TType> *pool, TType *ptr) : pool(pool), ptr(ptr) {}

				void	reset()
				{
					if (ptr != NULL)
					{
						ptr->~TType();
						pool->release(ptr);
						pool = NULL;
						ptr = NULL;
					}
				}

				void	swap(Object &other) noexcept
				{
					std::swap(pool, other.pool);
					std::swap(ptr, other.ptr);
				}

			public:
				Object() : pool(NULL), ptr(NULL) {}
				Object(const Object &) = delete;
				Object	&operator=(const Object &) = delete;
				~Object() { reset(); }

				Object(Object &&other) noexcept : pool(NULL), ptr(NULL) { swap(other); }

				Object	&operator=(Object &&other) noexcept
				{
					Object	tmp(std::move(other));

					swap(tmp);
					return (*this);
				}

				TType	*operator->() const { return (ptr); }
				TType	&operator*() const { return (*ptr); }
				explicit operator bool() const { return (ptr != NULL); }
		};

// ============================================================================================= //

		Pool() : freeList(NULL), usedCount(0) {}

		explicit Pool(const std::size_t &size) : freeList(NULL), usedCount(0)
		{
			resize(size);
		}

		~Pool()
		{
			std::size_t	i;

			if (usedCount != 0)
			{
				std::cerr << "Pool: destroyed while " << usedCount
					<< " object(s) still in use" << std::endl;
				std::abort();
			}
			i = 0;
			while (i < blocks.size())
			{
				::operator delete(blocks[i]);
				i++;
			}
		}

		Pool(const Pool &) = delete;
		Pool	&operator=(const Pool &) = delete;

// ============================================================================================= //

	private:
		void	addBlock()
		{
			Slot		*block;
			std::size_t	i;

			block = static_cast<Slot *>(::operator new(slt * sizeof(Slot)));
			try
			{
				blocks.push_back(block);
			}
			catch (...)
			{
				::operator delete(block);
				throw ;
			}
			i = 0;
			while (i < slt - 1)
			{
				block[i].next = &block[i + 1];
				i++;
			}
			block[slt - 1].next = freeList;
			freeList = block;
		}

	public:
		void	resize(const std::size_t &size)
		{
			if (size < usedCount)
				throw std::invalid_argument("Pool::resize: size is lower than the number of objects");
			while (capacity() < size)
				addBlock();
		}

// ============================================================================================= //

	private:
		void	release(TType *ptr)
		{
			Slot	*slot;

			slot = reinterpret_cast<Slot *>(ptr);
			slot->next = freeList;
			freeList = slot;
			usedCount--;
		}

	public:
		template <typename... TArgs>
		Object	acquire(TArgs &&... args)
		{
			Slot	*slot;
			Slot	*next;
			TType	*ptr;

			if (freeList == NULL)
				addBlock();
			slot = freeList;
			next = slot->next;
			ptr = new (static_cast<void *>(slot->storage)) TType(std::forward<TArgs>(args)...);
			freeList = next;
			usedCount++;
			return (Object(this, ptr));
		}

		std::size_t	capacity() const { return (blocks.size() * slt); }
		std::size_t	used() const { return (usedCount); }
		std::size_t	available() const { return (capacity() - usedCount); }
};

template <typename TType>
const std::size_t	Pool<TType>::slt;

#endif
