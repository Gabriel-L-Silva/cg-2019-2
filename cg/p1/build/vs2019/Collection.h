#ifndef __Collection_h
#define __Collection_h

#include <vector>
namespace cg
{
template <class T>
class Collection
{
public:
	void add(T object);
	void remove(T object);

	auto getIter() 
	{
		return _elements.begin();
	}
	auto getEnd() const
	{
		return _elements.end();
	}
	auto isEmpty()
	{
		return _elements.empty();
	}
private:
	std::vector<T> _elements;
};// Collection
}//end namespace cg

#endif // !__Collection_h