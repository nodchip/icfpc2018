#ifndef __UNION_FIND_H__
#define __UNION_FIND_H__

#include <vector>
using namespace std;
// copied from http://www.prefield.com/algorithm/container/union_find.html
struct UnionFind {
  vector<long long int> data;
UnionFind(long long int size) : data(size, -1) { }
  bool unionSet(long long int x, long long int y) {
    x = root(x); y = root(y);
    if (x != y) {
      if (data[y] < data[x]) swap(x, y);
      data[x] += data[y]; data[y] = x;
    }
    return x != y;
  }
  
  bool findSet(long long int x, long long int y) {
    return root(x) == root(y);
  }
  long long int root(long long int x) {
    return data[x] < 0 ? x : data[x] = root(data[x]);
  }
  long long int size(long long int x) {
    return -data[root(x)];
  }
};

#endif
