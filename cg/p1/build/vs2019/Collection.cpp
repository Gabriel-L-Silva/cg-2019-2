#include "Collection.h"

namespace cg
{
template<class T> void
Collection<T>::add(T object) {
	_elements.emplace_back(object);
}

template<class T> void
Collection<T>::remove(T object) {
	auto it = getIter();
	while (*it != object)
		it++;
	_elements.erase(it);
}

}