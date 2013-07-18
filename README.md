## Scala-style functional algorithms on C++ collections (Requires C++11)

[![Build Status](https://travis-ci.org/Navetas/Escalator.png)](https://travis-ci.org/Navetas/Escalator)


### Examples:

For:

```C++
std::vector<int> a = { 3, 1, 4, 4, 2 };
```

#### Square and sort a list of numbers


```C++
std::vector<int> res1 = lift(a)
    .map( []( int v ) { return v * v; } )
    .sortWith( []( int a, int b ) { return a < b; } )
    .toVec();
    
// Returns: 1, 4, 9, 16, 16
```
    
#### Return the index of all elements greater than two

```C++
std::vector<size_t> res2 = lift(a)
  .zipWithIndex()
  .filter( []( const std::pair<int, size_t>& v ) { return v.first > 2; } )
  .map( []( const std::pair<int, size_t>& v ) { return v.second; } )
  .toVec();
  
// Returns: 0, 2, 3
```

#### Return a sorted list of distinct numbers

```C++
std::set<int> res3 = lift(a)
    .toSet();
    
// Returns: 1, 2, 3, 4
```

#### Return a list of distinct numbers preserving order

```C++
set::vector<int> res4 = lift(a)
    .distinct()
    .toVec();
    
// Returns: 3, 1, 4, 2
```

#### Operations on numeric collections

```C++
std::vector<double> c = { 1.0, 2.0, 3.0, 4.0, 6.0, 7.0, 8.0, 4.9, 4.9, 5.2, 4.9, 4.9, 5.2, 9.0, 5.0 };

BOOST_CHECK_CLOSE( lift(c).mean(), 5.0, 1e-6 );
BOOST_CHECK_CLOSE( lift(c).median(), 4.9, 1e-6 );

BOOST_CHECK_CLOSE( lift(c).min(), 1.0, 1e-6 );
BOOST_CHECK_CLOSE( lift(c).max(), 9.0, 1e-6 );

BOOST_CHECK_EQUAL( std::get<0>( lift(c).argMin() ), 0 );
BOOST_CHECK_EQUAL( std::get<0>( lift(c).argMax() ), 13 );
```

#### Operations on strings

```C++
std::vector<int> els = { 1, 2, 3, 4, 5 };
std::string elements = " 1,  2, 3,  4,    5 ";
std::vector<int> tidied = lift(elements)
    .split(",")
    .map( []( const std::string& el )
    {
        return boost::lexical_cast<int>(lift(el).trim().toString());
    } )
    .toVec();
    
CHECK_SAME_ELEMENTS( els2, tidied );
```

#### Operations on streams

```C++
std::string lines( "1, 2 ,3  \n 4 ,5, 6\n7,8,9\n10,   11,12  " );
    
std::istringstream iss(lines);
std::vector<int> res = lift(iss)
    .map( []( const std::string& line )
    {
        auto els = lift(line).split(",").toVec();
        std::vector<int> numEls = els.map( []( const std::string& el )
        {
            return boost::lexical_cast<int>( lift(el).trim().toString() );
        } ).toVec();
        return numEls[1];
    } )
    .toVec();
    
CHECK_SAME_ELEMENTS( res, std::vector<int> { 2, 5, 8, 11 } );
```
