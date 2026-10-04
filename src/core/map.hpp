#pragma once

#include <vector>

enum class Tile : unsigned char { Wall, Floor };

class Map {
public:
    explicit Map(int width = 80, int height = 24);

    int width() const { return width_; }
    int height() const { return height_; }

    bool inBounds(int x, int y) const;
    Tile tile(int x, int y) const;          // вне карты -> Wall
    void setTile(int x, int y, Tile t);     // вне карты игнорируется
    bool walkable(int x, int y) const;
    bool opaque(int x, int y) const;        // вне карты -> true

    bool visible(int x, int y) const;
    bool explored(int x, int y) const;
    void setVisible(int x, int y, bool v);  // v=true также помечает explored
    void clearVisible();

private:
    int index(int x, int y) const { return y * width_ + x; }

    int width_;
    int height_;
    std::vector<Tile> tiles_;
    std::vector<char> visible_;
    std::vector<char> explored_;
};
