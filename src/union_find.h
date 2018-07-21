#ifndef __UNION_FIND_H__
#define __UNION_FIND_H__


struct UnionFind {
 public:
  UnionFind(int size) : data(size, -1) { }
  UnionFind(const nlohmann::json &json) {
    for (auto& j : json) {
      data.push_back(j.get<int>());
    }
  }

  nlohmann::json serialize() {
    nlohmann::json j = data;
    return j;
  }

  // x と y が同じグループに入れる。元々別のグループにいたら true を返す
  bool unionSet(int x, int y) {
    x = root(x);
    y = root(y);
    if (x != y) {
      if (data[y] < data[x])
        std::swap(x, y);

      cmd.push_back(x);
      cmd.push_back(data[x]);
      cmd.push_back(y);
      cmd.push_back(data[y]);

      data[x] += data[y];
      data[y] = x;
    } else {
      cmd.push_back(std::numeric_limits<int>::max());
    }
    return x != y;
  }
  // x と y が同じグループに入れば true
  bool findSet(int x, int y) {
    return root(x) == root(y);
  }
  // x の所属グループを返す
  int root(int x) {
    return data[x] < 0 ? x : root(data[x]);
  }
  // x と同じグループにいる要素数(x を含む)
  int size(int x) {
    return -data[root(x)];
  }

  void undo() {
    int data_y = cmd.back();
    cmd.pop_back();

    if (data_y == std::numeric_limits<int>::max()) {
      return;
    }

    int y = cmd.back();
    cmd.pop_back();
    int data_x = cmd.back();
    cmd.pop_back();
    int x = cmd.back();
    cmd.pop_back();
    data[x] = data_x;
    data[y] = data_y;
  }

 private:
  std::vector<int> data;
  std::vector<int> cmd;
};
