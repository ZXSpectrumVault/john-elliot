/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#ifndef ALGO_H
#define ALGO_H

#include <stdlib.h>
#ifndef __GNUG__
#include <ministl/bool.h>
#endif
#include <ministl/pair>

template <class InputIterator, class Function>
Function for_each(InputIterator first, InputIterator last, Function f) {
    while (first != last) f(*first++);
    return f;
}

template <class InputIterator, class T>
InputIterator find(InputIterator first, InputIterator last, const T& value) {
    while (first != last && *first != value) ++first;
    return first;
}

template <class InputIterator, class Predicate>
InputIterator find_if(InputIterator first, InputIterator last,
		      Predicate pred) {
    while (first != last && !pred(*first)) ++first;
    return first;
}

template <class ForwardIterator>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last) {
    if (first == last) return last;
    ForwardIterator next = first;
    while(++next != last) {
	if (*first == *next) return first;
	first = next;
    }
    return last;
}

template <class ForwardIterator, class BinaryPredicate>
ForwardIterator adjacent_find(ForwardIterator first, ForwardIterator last,
			      BinaryPredicate binary_pred) {
    if (first == last) return last;
    ForwardIterator next = first;
    while(++next != last) {
	if (binary_pred(*first, *next)) return first;
	first = next;
    }
    return last;
}

template <class InputIterator, class T, class Size>
void count(InputIterator first, InputIterator last, const T& value,
	   Size& n) {
    while (first != last) 
	if (*first++ == value) ++n;
}

template <class InputIterator, class Predicate, class Size>
void count_if(InputIterator first, InputIterator last, Predicate pred,
	      Size& n) {
    while (first != last)
	if (pred(*first++)) ++n;
}

#endif

