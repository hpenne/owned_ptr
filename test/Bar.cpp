//
// Created by Helge Penne on 01/11/2023.
//

#include "Bar.h"
#include "Foo.h"

Bar::Bar() : _foo{Foo{}} {
}

Bar::Bar(int value) : _value(value), _foo{Foo{}} {}
