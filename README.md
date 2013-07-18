
# Scala-style functional algorithms on C++ collections (Requires C++11)

[![Build Status](https://travis-ci.org/Navetas/Escalator.png)](https://travis-ci.org/Navetas/Escalator)


## Examples:

For:

```C++
std::vector<int> a = { 3, 1, 4, 4, 2 };
```

### Square and sort a list of numbers


```C++
std::vector<int> res1 = lift(a)
    .map( []( int v ) { return v * v; } )
    .sortWith( []( int a, int b ) { return a < b; } )
    .toVec();
    
// Returns: 1, 4, 9, 16, 16
```
    
### Return the index of all elements greater than two

```C++
std::vector<size_t> res2 = lift(a)
  .zipWithIndex()
  .filter( []( const std::pair<int, size_t>& v ) { return v.first > 2; } )
  .map( []( const std::pair<int, size_t>& v ) { return v.second; } )
  .toVec();
  
// Returns: 0, 2, 3
```

### Return a sorted list of distinct numbers

```C++
std::set<int> res3 = lift(a)
    .toSet();
    
// Returns: 1, 2, 3, 4
```

### Return a list of distinct numbers preserving order

```C++
set::vector<int> res4 = lift(a)
    .distinct()
    .toVec();
    
// Returns: 3, 1, 4, 2
```

    
