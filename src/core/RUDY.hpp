#pragma once

#include <iostream>
#include <vector>
#include <utility>
#include <unordered_map>

#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/networks/aig.hpp>
#include "assert.h"

namespace phyLS
{
using node_position = mockturtle::node_position;

struct Point
{
  Point() = default;
  Point(int x, int y) : _x(x), _y(y) {}

  int getX() const { return _x; }
  int getY() const { return _y; }
  int _x = 0;
  int _y = 0;
};

class Rect
{
 public:
  Rect() = default;
  Rect(const Rect& r) = default;
  Rect(int x1, int y1, int x2, int y2)
  {
    init(x1, y1, x2, y2);
  };

  Rect& operator=(const Rect& r) = default;
  bool operator==(const Rect& r) const;
  bool operator!=(const Rect& r) const { return !(*this == r); };
  bool operator<(const Rect& r) const;
  bool operator>(const Rect& r) const { return r < *this; }
  bool operator<=(const Rect& r) const { return !(*this > r); }
  bool operator>=(const Rect& r) const { return !(*this < r); }

  // Reinitialize the rectangle
  void init(int x1, int y1, int x2, int y2)
  {
    std::tie(_xlo, _xhi) = std::minmax(x1, x2);
    std::tie(_ylo, _yhi) = std::minmax(y1, y2);
  }

  // Reinitialize the rectangle without normalization
  void reset(int x1, int y1, int x2, int y2);

  // Moves the rectangle to the new point.
  void moveTo(int x, int y);

  // Moves the rectangle by the offset amount
  void moveDelta(int dx, int dy);

  void set_xlo(int x);
  void set_xhi(int x);
  void set_ylo(int y);
  void set_yhi(int y);

  int xMin() const { return _xlo; }
  int yMin() const { return _ylo; }
  int xMax() const { return _xhi; }
  int yMax() const { return _yhi; }
  int dx() const { return _xhi - _xlo; }
  int dy() const { return _yhi - _ylo; }
  int xCenter() const { return (_xlo + _xhi) / 2; }
  int yCenter() const { return (_ylo + _yhi) / 2; }

  // A point intersects the interior of this rectangle
  inline bool intersects(const Point& p) const
  {
    return (p.getX() >= _xlo) && (p.getX() <= _xhi) && (p.getY() >= _ylo)
          && (p.getY() <= _yhi);
  }

  // A rectangle intersects the interior of this rectangle
  inline bool intersects(const Rect& r) const
  {
    return (r._xhi >= _xlo) && (r._xlo <= _xhi) && (r._yhi >= _ylo)
          && (r._ylo <= _yhi);
  }

  // A rectangle intersects the interior of this rectangle
  bool overlaps(const Rect& r) const
  {
    return (r._xhi > _xlo) && (r._xlo < _xhi) && (r._yhi > _ylo)
          && (r._ylo < _yhi);
  }

  // Compute the intersection of these two rectangles.
  Rect intersect(const Rect& r) const
  {
    assert(intersects(r));
    Rect result;
    result._xlo = std::max(_xlo, r._xlo);
    result._ylo = std::max(_ylo, r._ylo);
    result._xhi = std::min(_xhi, r._xhi);
    result._yhi = std::min(_yhi, r._yhi);
    return result;
  }

  inline int64_t area() const {
    return dx() * static_cast<int64_t>(dy());
  }
  int64_t margin() const;

  void printf(FILE* fp, const char* prefix = "");
  void print(const char* prefix = "");

 private:
  int _xlo = 0;
  int _ylo = 0;
  int _xhi = 0;
  int _yhi = 0;
};

template <typename Ntk>
class RUDY
{
public:
  class Tile
  {
  public:
    Rect getRect() const { return _rect; }

    void setRect(int lx, int ly, int ux, int uy)
    {
      _rect = Rect(lx, ly, ux, uy);
    }

    void addRudy(double rudy) { _rudy += rudy; }
    double getRudy() const { return _rudy; }
    void setRudy(double rudy) { _rudy = rudy; }
    void clearRudy() { _rudy = 0.0; }

  private:
    Rect _rect;
    float _rudy = 0.0;
  };

  RUDY() = default;

  explicit RUDY(const std::vector<node_position>* placement, const Ntk* ntk, int num_pi, int num_po) : 
                                            _placement(placement), _ntk(ntk), _num_pi(num_pi), _num_po(num_po) 
  {
    // Set the wire width to 1 as default
    _wire_width = 12;

    // Build up the core grid
    std::cout << "Building core grid" << std::endl;
    buildCore();
    std::cout << "Building core grid done" << std::endl;
    makeGrid();
    std::cout << "Building grid done" << std::endl;
    extractNets();
  }

  void calculateRudy()
  {
    std::cout << "Calculating Rudy" << std::endl;
    for (auto& grid_colun : _grids)
    {
      for (auto& tile : grid_colun)
      {
        tile.clearRudy();
      }
    }

    // Consider all nets are 2-pin nets
    for (auto net : _nets)
    {
      computeNetsRudy(net);
    }
  }

  /**
   * Set the wire length for calculate Rudy.
   * If the layer which name is metal1 and it has getWidth value, then this
   * function will not applied, but it will apply that information.
   * */
  void setWireWidth(int wire_width) { _wire_width = wire_width; }

  void printGrids()
  {
    for (const auto& grid : _grids)
    {
      std::cout << "At grid: " << grid.getRect().xMin() << " " << grid.getRect().yMin() << " " << grid.getRect().xMax() << " " << grid.getRect().yMax() << " Rudy: " << grid.getRudy() << std::endl;
    }
  }

  void show()
  {
    int cur_x = 0;
    for (const auto& grid : _grids) {
      int cur_y = 0;
      for (const auto& tile : grid) {
        std::cout << "At grid: [" <<  cur_x << ", " << cur_y << "] Rudy: " << tile.getRudy() << std::endl;
        ++cur_y;
      }
      ++cur_x;
    }
  }

  void computeNetsRudy(std::pair<int, std::vector<int>> net_pins)
  {
    // The first element of the vector is the driver node, and the rest are the fanouts
    Rect net_rect;
    node_position drive_np = (*_placement)[net_pins.first];

    int xlo = drive_np.x_coordinate;
    int ylo = drive_np.y_coordinate;
    int xhi = drive_np.x_coordinate;
    int yhi = drive_np.y_coordinate;

    for (auto pin : net_pins.second)
    {
      node_position fanout_np = (*_placement)[pin];
      if (fanout_np.x_coordinate < xlo)
        xlo = fanout_np.x_coordinate;
      if (fanout_np.x_coordinate > xhi)
        xhi = fanout_np.x_coordinate;
      if (fanout_np.y_coordinate < ylo)
        ylo = fanout_np.y_coordinate;
      if (fanout_np.y_coordinate > yhi)
        yhi = fanout_np.y_coordinate;
    }
    net_rect.init(xlo - _wire_width / 2, ylo - _wire_width / 2, xhi + _wire_width / 2, yhi + _wire_width / 2);
    
    const auto net_area = net_rect.area();
    if (net_area == 0)
      std::cerr << "Error: Net area is zero" << std::endl;
    
    const auto hpwl = static_cast<float>(net_rect.dx() + net_rect.dy());
    const auto wire_area = hpwl * _wire_width;
    const auto net_congestion = wire_area / net_area;

    const int min_x_index
      = std::max(0, (net_rect.xMin() - _block_grid.xMin() ) / _tile_size);
    const int max_x_index = std::min(
        _tile_size_x - 1, (net_rect.xMax() - _block_grid.xMin()) / _tile_size);
    const int min_y_index
        = std::max(0, (net_rect.yMin() - _block_grid.yMin()) / _tile_size);
    const int max_y_index = std::min(
        _tile_size_y - 1, (net_rect.yMax() - _block_grid.yMin()) / _tile_size);

    // Iterate over the tiles in the calculated range
    for (int x = min_x_index; x <= max_x_index; ++x)
    {
      for (int y = min_y_index; y <= max_y_index; ++y)
      {
        Tile& tile = getGrid(x, y);
        const auto tile_box = tile.getRect();
        if (net_rect.overlaps(tile_box)) {
          const auto intersect_area = net_rect.intersect(tile_box).area();
          const auto tile_area = tile_box.area();
          const auto tile_net_box_ratio = static_cast<float>(intersect_area)
                                          / static_cast<float>(tile_area);
          const auto rudy = net_congestion * tile_net_box_ratio * 100;
          tile.addRudy(rudy);
        }
      }
    }
  }

  void set_tile_size(int tile_size) { _tile_size = tile_size; }

  // Return rudy at specified grid
  void getRudy(int x, int y) { return getGrid(x, y).getRudy(); }

 private:
 // Compute the area that only contains cells
  void buildCoreOnlyCell()
  {
    // In the vector of placement, the indices of PIs start from 1, The 0 indice represents constant
    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    int max_x = 0;
    int max_y = 0;

    // The first element of placement vector is constant node, and the latter elements are PIs
    for (auto const& node : *_placement)
    {
      if (node.x_coordinate > max_x)
        max_x = node.x_coordinate;
      if (node.x_coordinate < min_x)
        min_x = node.x_coordinate;
      if (node.y_coordinate > max_y)
        max_y = node.y_coordinate;
      if (node.y_coordinate < min_y)
        min_y = node.y_coordinate;
    }

    std::cout << "Core: " << min_x << " " << min_y << " " << max_x << " " << max_y << std::endl;
    _block_grid.init(min_x, min_y, max_x, max_y);
  }

  // compute the area of the entire core
  void buildCore()
  {
    // In the vector of placement, the indices of PIs start from 1, The 0 indice represents constant
    int min_x = std::numeric_limits<int>::max();
    int min_y = std::numeric_limits<int>::max();
    int max_x = 0;
    int max_y = 0;

    // The first element of placement vector is constant node, and the latter elements are PIs
    for (auto const& node : *_placement)
    {
      if (node.x_coordinate > max_x)
        max_x = node.x_coordinate;
      if (node.x_coordinate < min_x)
        min_x = node.x_coordinate;
      if (node.y_coordinate > max_y)
        max_y = node.y_coordinate;
      if (node.y_coordinate < min_y)
        min_y = node.y_coordinate;
    }

    std::cout << "Core: " << min_x << " " << min_y << " " << max_x << " " << max_y << std::endl;
    _block_grid.init(min_x, min_y, max_x, max_y);
  }

  // Extract the nets from the AIG network and its companion placement
  void extractNets()
  {
    // Extract nets from the network, each key denotes a net, named by its drive
    _ntk->foreach_gate([&](auto n) {
      auto nindex = _ntk->node_to_index(n);
      _ntk->foreach_fanin(n, [&](auto f) {
        auto index = _ntk->node_to_index(_ntk->get_node(f));
        _nets[index].push_back(nindex);
      });
    });
  }
  
  void compute_tile_cnt()
  {
    int width_x = _block_grid.xMax() - _block_grid.xMin();
    int width_y = _block_grid.yMax() - _block_grid.yMin();
    _tile_size_x = width_x / _tile_size;
    _tile_size_y = width_y / _tile_size;
  }

  void makeGrid()
  {
    compute_tile_cnt();
    std::cout << "Tile size: " << _tile_size_x << " " << _tile_size_y << std::endl;
    const int grid_lx = _block_grid.xMin();
    const int grid_ly = _block_grid.yMin();

    int vertical_margin = _block_grid.xMax() - _tile_size_y * _tile_size;
    int horizontal_margin = _block_grid.yMax() - _tile_size_x * _tile_size;

    _grids.resize(_tile_size_x);
    int cur_x = 0;
    for (int x = 0; x < _tile_size_x; x++)
    {
      _grids[x].resize(_tile_size_y);
      int cur_y = 0;
      for (int y = 0; y < _tile_size_y; y++)
      {
        Tile& grid = _grids[x][y];
        int x_ext = x == _grids.size() - 1 ? horizontal_margin : 0;
        int y_ext = y == _grids[x].size() - 1 ? vertical_margin : 0;
        grid.setRect(
            cur_x, cur_y, cur_x + _tile_size + x_ext, cur_y + _tile_size + y_ext);
        cur_y += _tile_size;
      }
      cur_x += _tile_size;
    }
  }

  Tile& getGrid(int x, int y) { return _grids[x][y]; }

  // It corresponds to the vector match position in mapper
  const std::vector<node_position>* _placement;
  const Ntk* _ntk;
  int _num_pi;
  int _num_po;

  Rect _block_grid;
  std::vector<std::vector<Tile>> _grids;
  std::unordered_map<int, std::vector<int>> _nets;
  int _wire_width = 0;
  int _tile_size_x = 10;
  int _tile_size_y = 10;
  int _tile_size = 600; // should we respectively set horizontal and vertical tile size?
};

} // namespace RUDY