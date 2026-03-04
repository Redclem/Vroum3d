#ifndef VROUM3D_BAG_HPP_INCLUDED
#define VROUM3D_BAG_HPP_INCLUDED

/** Bag container class, just contains a variable amount of elements.
 * Access through pointer only. Insertion / Removal in O(1)
*/

#include "utility.h"
#include <memory>

namespace Vroum3d
{

template<typename T, typename Allocator = std::allocator<T>>
class Bag : public Allocator
{
public:
	using element_t = T;
	using allocator_t = Allocator;
	using allocator_traits_t = std::allocator_traits<allocator_t>;
private:

	static constexpr std::size_t subbag_size = 1024;


	union SubbagElement
	{
		SubbagElement* next;
		element_t element;

		SubbagElement() {}
	};

	struct Subbag
	{
		std::array<element_t, subbag_size> sb_elems;
		Subbag* next;
	};

	Handle<Subbag*, nullptr> m_subbag;
	SubbagElement * m_next_free = nullptr;
	//allocator_t m_allocator;
public:

	Bag(allocator_t&& all = {}) : allocator_t(all) {}

	void destroy()
	{
		for(auto sb = m_subbag; sb; sb = sb->next)
			delete sb;
	}

	template<typename PtrType>
	class ConstPtr
	{
	protected:
		using ptr_t = PtrType;
		ptr_t tget;

		friend class Bag;
		ConstPtr(ptr_t t) : tget(t) {}
	public:
		const auto& operator*() const {return *tget;}
		const auto* operator->() const {return tget;}
	};

	template<typename PtrType>
	class Ptr : public ConstPtr<PtrType>
	{
		using base_t = ConstPtr<PtrType>;
		using base_t::base_t, typename base_t::ptr_t;

		friend class Bag;
		Ptr(ptr_t t) : base_t::base_t(t) {}
	public:
		auto& operator*() const {return *base_t::tget;}
		auto* operator->() const {return base_t::tget;}
	};

	using const_ptr_t = ConstPtr<const element_t*>;
	using ptr_t = Ptr<element_t*> ;

private:

	auto& alloc() {return static_cast<allocator_t&>(*this);}

	// Use when no new free element (m_next_free == nullptr)
	void add_subbag()
	{
		Subbag* new_subb = new Subbag();
		new_subb->next = m_subbag;
		m_subbag = new_subb;

		for(auto iter = m_subbag->sb_elems.begin(), end = std::prev(m_subbag->sb_elems.end());
      			iter != end;)
		{
			iter->next = &*(++iter);
		}
		m_subbag->sb_elems.back().next = nullptr;
		m_next_free = m_subbag->sb_elems.front();
	}

public:
	ptr_t allocate()
	{
		if(m_next_free == nullptr)
			add_subbag();

		auto newelem = m_next_free;
		m_next_free = m_next_free->next;
		allocator_traits_t::construct(alloc(), newelem->element);
		return ptr_t(newelem);
	}

	void release(ptr_t p)
	{
		allocator_traits_t::destroy(alloc(), p->element);
		p->next = m_next_free;
		m_next_free = p;
	}
};


}

#endif
