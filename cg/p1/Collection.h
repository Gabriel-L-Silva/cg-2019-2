#ifndef __Collection_h
#define __Collection_h

#include <vector>
namespace cg
{
template <class T>
class Collection
{
public:
	Collection() {}

	auto add(T object) {
		_elements.emplace_back(object);
	}

	auto remove(T object) {
		auto it = getIter();
		while (*it != object)
			it++;
		_elements.erase(it);
	}


	auto getIter()
	{
		return _elements.begin();
	}

	auto getEnd()
	{
		return _elements.end();
	}

	auto isEmpty()
	{
		return _elements.empty();
	}

	auto clear()
	{
		_elements.clear();
	}

private:
	std::vector<T> _elements;
};// Collection
}//end namespace cg

#endif // !__Collection_h