#ifndef VROUM3D_BAG_HPP_INCLUDED
#define VROUM3D_BAG_HPP_INCLUDED

/** Bag container class, just contains a variable amount of elements.
 * Access through pointer only. Insertion / Removal in O(1).
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
	static constexpr std::size_t c_subbag_size = 1024;
private:



	union SubbagElement
	{
		SubbagElement* next;
		element_t element;

		SubbagElement() {}
	};

	struct Subbag
	{
		std::array<SubbagElement, c_subbag_size> sb_elems;
		Subbag* next;
	};

	Handle<Subbag*, nullptr> m_subbag;
	SubbagElement * m_next_free = nullptr;
  std::size_t m_idx_new_in_subbag = c_subbag_size;

public:

	Bag(allocator_t&& all = {}) : allocator_t(all) {}

	void destroy()
	{
		for(auto sb = m_subbag; sb;)
    {
     auto tmp = sb->next;
			delete sb;
      sb = tmp;
    }
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
    ConstPtr(std::nullptr_t = nullptr) {}
		const auto& operator*() const {return tget->element;}
		const auto* operator->() const {return &tget->element;}
	};

	template<typename PtrType>
	class Ptr : public ConstPtr<PtrType>
	{
		using base_t = ConstPtr<PtrType>;
		using base_t::base_t, typename base_t::ptr_t;

		friend class Bag;
		Ptr(ptr_t t) : base_t(t) {}
	public:
    Ptr(std::nullptr_t = nullptr) {}
		auto& operator*() const {return base_t::tget->element;}
		auto* operator->() const {return &base_t::tget->element;}
	};

	using const_ptr_t = ConstPtr<const SubbagElement*>;
	using ptr_t = Ptr<SubbagElement*> ;

private:

	auto& alloc() {return static_cast<allocator_t&>(*this);}

	// Use when no new free element (m_next_free == nullptr)
	void add_subbag()
	{
		Subbag* new_subb = new Subbag();
		new_subb->next = m_subbag;
		m_subbag = new_subb;
	  m_idx_new_in_subbag = 0;
  }

  SubbagElement* allocate_free()
  {
    auto newelem = m_next_free;
    m_next_free = m_next_free->next;
    return newelem;
  }

  SubbagElement* allocate_in_subbag()
  {
    if(m_idx_new_in_subbag == c_subbag_size)
      add_subbag();

    auto* newelem = m_subbag->sb_elems.data() + m_idx_new_in_subbag;
    m_idx_new_in_subbag++;
    return newelem;
  }

public:
	ptr_t allocate()
	{
    SubbagElement * p;
    if(m_next_free)
      p = allocate_free();
    else
      p = allocate_in_subbag();

    allocator_traits_t::construct(alloc(), &p->element);
    return ptr_t(p);
  }

	void release(ptr_t ptr)
	{
    auto p = ptr.tget;
		allocator_traits_t::destroy(alloc(), &p->element);
		p->next = m_next_free;
		m_next_free = p;
	}

  std::size_t n_subbags() const
  {
    std::size_t ns(0);
    for(Subbag* iter = m_subbag; iter; iter = iter->next)
      ns++;

    return ns;
  }
};


}

#endif
