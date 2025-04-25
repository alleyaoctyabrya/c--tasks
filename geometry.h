#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <iostream>
#include <vector>

const double eps = 1e-4;

struct Point {
  double x = 0;
  double y = 0;

  Point() = default;

  Point(const double first, const double second) : x(first), y(second) {}

  void rotate(double angle) {
    double sinus = sin(M_PI * angle / 180);
    double cosinus = cos(M_PI * angle / 180);
    double _x = x;
    double _y = y;
    x = cosinus * _x - sinus * _y;
    y = sinus * _x + cosinus * _y;
  }

  Point &operator-=(const Point &another) {
    x -= another.x;
    y -= another.y;
    return *this;
  }

  Point &operator+=(const Point &another) {
    x += another.x;
    y += another.y;
    return *this;
  }

  Point operator*=(double value) {
    x *= value;
    y *= value;
    return *this;
  }

  Point operator/=(double value) {
    x /= value;
    y /= value;
    return *this;
  }

  void rotate(const Point &center, double angle) {
    operator-=(center);
    rotate(angle);
    operator+=(center);
  }

  bool operator==(const Point &point) const {
    if (std::fabs(x - point.x) < eps && std::fabs(y - point.y) < eps) {
      return true;
    }
    return false;
  }

  bool operator!=(const Point &point) const {
    return !(*this == point);
  }
};

bool compare(const Point &first, const Point &second) {
  return (first.x == second.x ? first.y < second.y : first.x < second.x);
}

Point operator-(const Point &first, const Point &second) {
  Point temp = first;
  temp -= second;
  return temp;
}

Point operator+(const Point &first, const Point &second) {
  Point temp = first;
  temp += second;
  return temp;
}

Point operator*(const Point &first, const double value) {
  Point temp = first;
  temp *= value;
  return temp;
}

double operator*(const Point &first, const Point &second) {
  return first.x * second.x + first.y * second.y;
}

Point operator/(const Point &first, const double value) {
  Point temp = first;
  temp /= value;
  return temp;
}

class Line {
public:
  double A = 0;
  double B = 0;
  double C = 0;

  Line() = default;

  Line(double k_first, double k_second, double k_third) : A(k_first), B(k_second), C(k_third) {}

  Line(const Point &f_point, const Point &s_point) {
    A = s_point.y - f_point.y;
    B = f_point.x - s_point.x;
    C = s_point.x * f_point.y - s_point.y * f_point.x;
    change();
  }

  Line(const double &coef, const double &shift) {
    A = -coef;
    B = 1;
    C = -shift;
    change();
  }

  Line(const Point &point, const double &coef) {
    A = -coef;
    B = 1;
    C = -(point.y - coef * point.x);
    change();
  }

  void change() {
    if (B != 0) {
      A /= B;
      C /= B;
      B = 1;
    } else {
      B /= A;
      C /= A;
      A = 1;
    }
  }

  bool operator==(const Line &line) const {
    if (std::fabs(A - line.A) < eps && std::fabs(B - line.B) < eps && std::fabs(C - line.C) < eps) {
      return true;
    }
    return false;
  }

  bool operator!=(const Line &line) const {
    return !(*this == line);
  }

  bool half_plane(const Point &point, const Point &s_point) const {
    bool first = A * point.x + B * point.y + C >= 0;
    bool second = A * s_point.x + B * s_point.y + C >= 0;
    return (first == second);
  }
};

Point intersection(const Line &first, const Line &second) {
  double det = first.A * second.B - second.A * first.B;
  double det_a = (-first.C) * second.B - (-second.C) * first.B;
  double det_b = first.A * (-second.C) - second.A * (-first.C);
  return {det_a / det, det_b / det};
}

namespace functions {
  void reflect_point(Point &one, const Point &center) {
    one.x = 2 * center.x - one.x;
    one.y = 2 * center.y - one.y;
  }

  double len(const Point &point) {
    return sqrt(point.x * point.x + point.y * point.y);
  }

  void reflect_point_axis(Point &one, const Line &axis) {
    double delta = pow(axis.A, 2) + pow(axis.B, 2);
    double free_term = -axis.A * one.y + axis.B * one.x;
    double delta_x = -axis.A * axis.C + axis.B * free_term;
    double delta_y = -axis.A * free_term - axis.B * axis.C;
    Point temp_point(delta_x / delta, delta_y / delta);
    functions::reflect_point(one, temp_point);
  }

  void scale_point(Point &one, const Point &center, double coefficient) {
    Point vec = one - center;
    vec *= coefficient;
    one = center + vec;
  }

  double check_turn(Point f_point, Point s_point, Point t_point) {
    Point ab = {
      s_point.x - f_point.x,
      s_point.y - f_point.y
    };
    Point bc = {
      t_point.x - s_point.x,
      t_point.y - s_point.y
    };
    double turn = ab.x * bc.y - ab.y * bc.x;
    return turn;
  }
}

class Shape {
public:
  virtual double area() const = 0;

  virtual bool isCongruentTo(const Shape &another) const = 0;

  virtual double perimeter() const = 0;

  virtual bool operator==(const Shape &another) const = 0;

  virtual void reflect(const Line &axis) = 0;

  virtual void reflect(const Point &center) = 0;

  virtual void scale(const Point &center, double coefficient) = 0;

  virtual void rotate(const Point &center, double angle) = 0;

  virtual bool isSimilarTo(const Shape &another) const = 0;

  virtual bool containsPoint(const Point &point) const = 0;

  virtual ~Shape() = default;

  bool operator!=(const Shape &another) const {
    return !(*this == another);
  }
};

class Polygon : public Shape {
protected:
  std::vector<Point> points;
public:
  Polygon() = default;

  Polygon(const std::vector<Point> &polygon) : points(polygon) {}

  Polygon(std::initializer_list<Point> _points) : points(_points) {}

  template<typename... T>
  Polygon(T &... points) : Polygon({points...}) {}

  size_t verticesCount() const {
    return points.size();
  }

  bool containsPoint(const Point &point) const override {
    double rotate_angle = 0;
    for (size_t i = 0; i < points.size(); ++i) {
      Point f_vec = point - points[i];
      Point s_vec = point - points[(i + 1) % points.size()];
      rotate_angle += acos(f_vec * s_vec / (functions::len(f_vec) * functions::len(s_vec))) *
                      ((f_vec.x * s_vec.y - f_vec.y * s_vec.x) >= 0 ? 1 : -1);
    }
    return std::fabs(rotate_angle) > eps;
  }

  bool isSimilarTo(const Shape &another) const override {
    const auto *polygon = dynamic_cast<const Polygon *> (&another);
    if (polygon == nullptr) {
      return false;
    }
    return (polygon->verticesCount() == verticesCount()) && (std::fabs(
      polygon->area() / area() - (polygon->perimeter() / perimeter()) * (polygon->perimeter() / perimeter())) < eps);
  }

  bool isCongruentTo(const Shape &another) const override {
    const auto *polygon = dynamic_cast<const Polygon *> (&another);
    if (polygon == nullptr) {
      return false;
    }
    return (polygon->verticesCount() == verticesCount()) && (std::fabs(polygon->area() - area()) < eps) &&
           (std::fabs(polygon->perimeter() - perimeter()) < eps);
  }

  double area() const override {
    double sum = 0;
    Point first = points[0];
    Point second = points[points.size() - 1];
    for (size_t i = 0; i < points.size(); ++i) {
      first = second;
      second = points[i];
      sum += first.x * second.y - first.y * second.x;
    }
    return 0.5 * std::fabs(sum);
  }

  void scale(const Point &center, double coefficient) override {
    for (auto &point: points) {
      functions::scale_point(point, center, coefficient);
    }
  }

  void reflect(const Line &axis) override {
    for (auto &point: points) {
      functions::reflect_point_axis(point, axis);
    }
  }

  void rotate(const Point &center, double angle) override {
    for (auto &point: points) {
      point.rotate(center, angle);
    }
  }

  void reflect(const Point &center) override {
    for (auto &point: points) {
      functions::reflect_point(point, center);
    }
  }

  double perimeter() const override {
    double sum = functions::len(points[0] - points[points.size() - 1]);
    for (size_t i = 0; i < points.size() - 1; ++i) {
      sum += functions::len(points[i] - points[i + 1]);
    }
    return sum;
  }

  bool operator==(const Shape &another) const final {
    const auto *polygon = dynamic_cast<const Polygon *> (&another);
    if (polygon == nullptr) {
      return false;
    }
    if (polygon->points.size() != points.size()) {
      return false;
    }
    std::vector<Point> sort_points = points;
    sort(sort_points.begin(), sort_points.end(), compare);
    std::vector<Point> other_sort_points = polygon->points;
    sort(other_sort_points.begin(), other_sort_points.end(), compare);
    for (size_t i = 0; i < points.size(); ++i) {
      if (std::fabs(sort_points[i].x - other_sort_points[i].x) > eps ||
          std::fabs(sort_points[i].y - other_sort_points[i].y) > eps) {
        return false;
      }
    }
    return true;
  }

  std::vector<Point> getVertices() const {
    return points;
  }

  bool isConvex() const {
    int size = points.size();
    if (size >= 3) {
      double turn = functions::check_turn(points[size - 1], points[0], points[1]);
      for (int i = 1; i < size - 1; ++i) {
        double temp_turn = functions::check_turn(points[i - 1], points[i], points[i + 1]);
        if (temp_turn * turn < 0) {
          return false;
        }
      }
      double last_turn = functions::check_turn(points[size - 2], points[size - 1], points[0]);
      if (last_turn * turn < 0) {
        return false;
      }
    }
    return true;
  }
};

class Ellipse : public Shape {
protected:
  double a = 0;
  double b = 0;
  double c = 0;
  Point first_focus = {};
  Point second_focus = {};
  double sum = 0;
public:
  Ellipse() = default;

  Ellipse(const Point &first, const Point &second, const double &sum) : first_focus(first), second_focus(second),
                                                                        sum(sum) {
    a = sum / 2;
    c = functions::len(first_focus - second_focus) / 2;
    b = sqrt(pow(a, 2) - pow(c, 2));
  }

  double area() const override {
    return M_PI * a * b;
  }

  bool containsPoint(const Point &point) const override {
    return functions::len(first_focus - point) + functions::len(second_focus - point) <= sum;
  }

  void reflect(const Line &axis) override {
    functions::reflect_point_axis(first_focus, axis);
    functions::reflect_point_axis(second_focus, axis);
  }

  bool isSimilarTo(const Shape &another) const override {
    const Ellipse *ellipse = dynamic_cast<const Ellipse *> (&another);
    if (ellipse == nullptr) {
      return false;
    }
    return std::fabs((a / b) - (ellipse->a / ellipse->b)) < eps;
  }

  bool isCongruentTo(const Shape &another) const override {
    const Ellipse *ellipse = dynamic_cast<const Ellipse *> (&another);
    if (ellipse == nullptr) {
      return false;
    }
    return std::fabs(ellipse->a - a) < eps &&
           std::fabs(ellipse->b - b) < eps &&
           std::fabs(ellipse->sum - sum) < eps;
  }

  void reflect(const Point &center) override {
    functions::reflect_point(first_focus, center);
    functions::reflect_point(second_focus, center);
  }

  void rotate(const Point &center, double angle) override {
    first_focus.rotate(center, angle);
    second_focus.rotate(center, angle);
  }

  void scale(const Point &center, double coefficient) override {
    functions::scale_point(first_focus, center, coefficient);
    functions::scale_point(second_focus, center, coefficient);
    a *= coefficient;
    b *= coefficient;
    c *= coefficient;
  }

  double perimeter() const override {
    return M_PI * (3 * (a + b) - sqrt((3 * a + b) * (a + 3 * b)));
  }

  std::pair<Point, Point> focuses() const {
    return {first_focus, second_focus};
  }

  double eccentricity() const {
    return sqrt((1 - pow(b / a, 2)));
  }

  Point center() const {
    return {(first_focus.x + second_focus.x) / 2, (first_focus.y + second_focus.y) / 2};
  }

  std::pair<Line, Line> directrices() const {
    return {Line(0, 1, -(pow(a, 2) / eccentricity())),
            Line(0, 1, (pow(a, 2) / eccentricity()))};
  }

  bool operator==(const Shape &another) const override {
    const Ellipse *ellipse = dynamic_cast<const Ellipse *> (&another);
    if (ellipse == nullptr) {
      return false;
    }
    std::vector<Point> f_sort = {first_focus, second_focus};
    sort(f_sort.begin(), f_sort.end(), compare);
    std::vector<Point> s_sort = {ellipse->first_focus, ellipse->second_focus};
    sort(s_sort.begin(), s_sort.end(), compare);
    if ((std::fabs(f_sort[0].x - s_sort[0].x) < eps &&
         std::fabs(f_sort[0].y - s_sort[0].y) < eps) &&
        (std::fabs(f_sort[1].x - s_sort[1].x) < eps &&
         std::fabs(f_sort[1].y - s_sort[1].y) < eps)) {
      return true;
    }
    return false;
  }
};

class Circle : public Ellipse {
public:
  Circle() = default;

  Circle(Point center, double radius) : Ellipse(center, center, 2 * radius) {}

  double radius() const {
    return a;
  }
};

class Rectangle : public Polygon {
public:
  using Polygon::Polygon;

  Rectangle(Point f_point, Point s_point, double _ratio) : Polygon(f_point, f_point, s_point, s_point) {
    _ratio = (_ratio < 1 ? 1 / _ratio : _ratio);
    double angle = atan(_ratio);
    Point t_point = s_point - f_point;
    t_point *= cos(angle);
    t_point.rotate(angle * 180 / M_PI);
    t_point += f_point;
    Point mid = (f_point + s_point) / 2;
    Point fth_point = mid * 2 - t_point;
    Polygon::points[1] = t_point;
    Polygon::points[3] = fth_point;
  }

  Point center() const {
    return (Polygon::points[0] + Polygon::points[2]) / 2;
  }

  std::pair<Line, Line> diagonals() const {
    return {Line(Polygon::points[0], Polygon::points[2]), Line(Polygon::points[1], Polygon::points[3])};
  }
};

class Square : public Rectangle {
public:
  Square() = default;

  Square(const Point &first, const Point &second) : Rectangle(first, second, 1) {}

  Circle circumscribedCircle() {
    Point vec = Rectangle::points[0] - Rectangle::points[2];
    double len = sqrt(pow(vec.x, 2) + pow(vec.y, 2));
    return {(Rectangle::points[0] + Rectangle::points[2]) / 2, len / 2};
  }

  Circle inscribedCircle() {
    Point vec = Rectangle::points[0] - Rectangle::points[2];
    double len = sqrt(pow(vec.x, 2) + pow(vec.y, 2));
    return {(Rectangle::points[0] + Rectangle::points[2]) / 2, len / (sqrt(2) * 2)};
  }
};

class Triangle : public Polygon {
public:
  using Polygon::Polygon;

  Triangle() = default;

  Circle circumscribedCircle() const {
    double x_f = Polygon::points[0].x;
    double y_f = Polygon::points[0].y;
    double x_s = Polygon::points[1].x;
    double y_s = Polygon::points[1].y;
    double x_t = Polygon::points[2].x;
    double y_t = Polygon::points[2].y;
    double expr = (y_f * (pow(x_s, 2) + pow(y_s, 2)
                          - pow(x_t, 2) - pow(y_t, 2)) + y_s * (pow(x_t, 2) + pow(y_t, 2)
                                                                - pow(x_f, 2) - pow(y_f, 2)) +
                   y_t * (pow(x_f, 2) + pow(y_f, 2)
                          - pow(x_s, 2) - pow(y_s, 2))) / (x_f * (y_s - y_t) + x_s * (y_t - y_f)
                                                           + x_t * (y_f - y_s));
    double s_expr = (x_f * (pow(x_s, 2) + pow(y_s, 2)
                            - pow(x_t, 2) - pow(y_t, 2)) + x_s * (pow(x_t, 2) + pow(y_t, 2)
                                                                  - pow(x_f, 2) - pow(y_f, 2)) +
                     x_t * (pow(x_f, 2) + pow(y_f, 2)
                            - pow(x_s, 2) - pow(y_s, 2))) / (x_f * (y_s - y_t) + x_s * (y_t - y_f)
                                                             + x_t * (y_f - y_s));
    Point center = {-expr / 2, s_expr / 2};
    double f_side = functions::len(Polygon::points[0] - Polygon::points[1]);
    double s_side = functions::len(Polygon::points[1] - Polygon::points[2]);
    double t_side = functions::len(Polygon::points[2] - Polygon::points[0]);
    double p = (f_side + s_side + t_side) / 2;
    double R = (f_side * s_side * t_side) / (4 * sqrt(p * (p - f_side) *
                                                      (p - s_side) * (p - t_side)));
    return {center, R};
  }

  Circle inscribedCircle() const {
    double f_side = functions::len(Polygon::points[0] - Polygon::points[1]);
    double s_side = functions::len(Polygon::points[1] - Polygon::points[2]);
    double t_side = functions::len(Polygon::points[2] - Polygon::points[0]);
    double y = (s_side * Polygon::points[0].y + t_side * Polygon::points[1].y + f_side * Polygon::points[2].y) /
               (f_side + s_side + t_side);
    double x = (s_side * Polygon::points[0].x + t_side * Polygon::points[1].x + f_side * Polygon::points[2].x) /
               (f_side + s_side + t_side);
    double r = 2 * area() / (f_side + s_side + t_side);
    return {{x, y}, r};
  }

  Point centroid() const {
    return (Polygon::points[0] + Polygon::points[1] + Polygon::points[2]) / 3;
  }

  Point orthocenter() const {
    Point first_vec = Polygon::points[1] - Polygon::points[0];
    first_vec.rotate(90);
    Line first(first_vec.y, -first_vec.x, -first_vec.y * Polygon::points[2].x + Polygon::points[2].y * first_vec.x);

    Point second_vec = Polygon::points[2] - Polygon::points[1];
    second_vec.rotate(90);
    Line second(second_vec.y, -second_vec.x,
                -second_vec.y * Polygon::points[0].x + Polygon::points[0].y * second_vec.x);
    return intersection(first, second);
  }

  Line EulerLine() const {
    return {orthocenter(), centroid()};
  }

  Circle ninePointsCircle() const {
    Point orth = orthocenter();
    Circle circle = circumscribedCircle();
    return {(circle.center() + orth) / 2, circle.radius() / 2};
  }
};
