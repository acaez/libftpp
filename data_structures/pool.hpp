#ifndef POOL_HPP
# define POOL_HPP

# include <cstddef>
# include <cstdlib>
# include <iostream>
# include <new>
# include <stdexcept>
# include <utility>
# include <vector>

template <typename TType>

class Pool
{
	private:
		union Slot
		{
			Slot							*next;
			alignas(TType) unsigned char	storage[sizeof(TType)];
		};

	public:
		static const std::size_t    slt = 32;

		class Object
		{
            private:
                Pool<TType> *pool;
                TType       *ptr;
				friend class Pool<TType>;
                Object(Pool<TType> *pool, TType *ptr) : pool(pool), ptr(ptr) {}
                
                void    reset()
                {
                    if (ptr != NULL)
                    {
                        ptr->~TType();
                        pool->release(ptr);
                        pool = NULL;
                        ptr = NULL;
                    }
                }
                
                void    swap(Object &other) noexcept
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
                    Object tmp(std::move(other));
                    swap(tmp);
                    return (*this);
                }

				TType	*operator->() const { return (ptr); }
				TType	&operator*() const { return (*ptr); }


 // =============================================================================================== //
		
        Pool() : _freeList(NULL), _used(0) {}

		explicit Pool(const std::size_t &p_size) : _freeList(NULL), _used(0)
		{
			resize(p_size);
		}

		~Pool()
		{
			if (_used != 0)
			{
				std::cerr << "Pool: destroyed while " << _used
					<< " object(s) still in use" << std::endl;
				std::abort();
			}
			for (std::size_t i = 0; i < _blocks.size(); ++i)
				::operator delete(_blocks[i]);
		}

		// Les Object gardent un pointeur vers leur Pool : ni copie ni
		// deplacement, sinon ce pointeur deviendrait invalide.
		Pool(const Pool &) = delete;
		Pool	&operator=(const Pool &) = delete;

		// Garantit au moins p_size emplacements au total, en ajoutant des
		// blocs. Ne retire jamais de bloc.
		void	resize(const std::size_t &p_size)
		{
			if (p_size < _used)
				throw std::invalid_argument(
					"Pool::resize: size is lower than the number of objects in use");
			while (capacity() < p_size)
				_addBlock();
		}

		// Construit un TType dans un emplacement libre. Ajoute un bloc si le
		// Pool est plein.
		template <typename... TArgs>
		Object	acquire(TArgs &&... p_args)
		{
			if (_freeList == NULL)
				_addBlock();

			Slot	*slot = _freeList;
			Slot	*next = slot->next;
			// Si le constructeur de TType leve une exception, la free list
			// n'a pas encore ete modifiee : rien a annuler.
			TType	*ptr = new (static_cast<void *>(slot->storage))
				TType(std::forward<TArgs>(p_args)...);
			_freeList = next;
			++_used;
			return (Object(this, ptr));
		}

		std::size_t	capacity() const { return (_blocks.size() * BLOCK_SIZE); }
		std::size_t	used() const { return (_used); }
		std::size_t	available() const { return (capacity() - _used); }

	private:
		// Alloue un bloc brut (aucun TType construit) et chaine ses cases en
		// tete de la free list.
		void	_addBlock()
		{
			Slot	*block = static_cast<Slot *>(
				::operator new(BLOCK_SIZE * sizeof(Slot)));

			try
			{
				_blocks.push_back(block);
			}
			catch (...)
			{
				::operator delete(block);
				throw ;
			}
			for (std::size_t i = 0; i < BLOCK_SIZE - 1; ++i)
				block[i].next = &block[i + 1];
			block[BLOCK_SIZE - 1].next = _freeList;
			_freeList = block;
		}

		// Remet un emplacement (objet deja detruit) en tete de la free list.
		void	_release(TType *p_ptr)
		{
			Slot	*slot = reinterpret_cast<Slot *>(p_ptr);

			slot->next = _freeList;
			_freeList = slot;
			--_used;
		}

		std::vector<Slot *>	_blocks;
		Slot				*_freeList;
		std::size_t			_used;
};

template <typename TType>
const std::size_t	Pool<TType>::BLOCK_SIZE;

#endif
