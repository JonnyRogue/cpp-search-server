#pragma once
#include <algorithm>
#include <vector>
#include <cassert>
#include <iostream>
#include "document.h"
 
using namespace std::string_literals;

template <typename IteratorRangee>
class IteratorRange{
public:
IteratorRange(IteratorRangee beginnn,IteratorRangee enddd)
:begin_(beginnn),end_(enddd){}

IteratorRangee begin() const {
return begin_;
}

IteratorRangee end() const {
return end_;
}

size_t size() const {
return abs(distance(begin_,end_));;
}


private:
IteratorRangee begin_;
IteratorRangee end_;
};

template <typename Iterator>
class Paginator {
public:
Paginator(Iterator begin_, Iterator end_, size_t page_size)
{
Iterator i = begin_;
while (distance(i,end_)>page_size){
    Iterator m = i;
    i+=page_size;
    page_.push_back(IteratorRange(m,i));
}
if (distance(i,end_)>0){
page_.push_back(IteratorRange(i,end_));
}
}

auto begin() const {
return page_.begin();
}
auto end() const {
return page_.end();
}
auto size() {
return page_.size();
}


private:
std::vector<IteratorRange<Iterator>> page_;
}; 

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, IteratorRange<Iterator> itRange) {
    for (auto doc: itRange) {
        os << doc;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const Document & doc) {
    os << "{ document_id = "s << doc.id
       << ", relevance = " << doc.relevance
       << ", rating = " << doc.rating
       << " }";
    return os;
}


template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
