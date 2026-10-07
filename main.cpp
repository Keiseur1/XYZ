#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCompleter>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPair>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSet>
#include <QSlider>
#include <QStatusBar>
#include <QTableWidget>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

// ==================== CẤU TRÚC DỮ LIỆU ĐỒ THỊ ====================
struct Node {
  int id;
  QString name;
  QPointF pos;
  QString category; // "poi", "bridge", "station", "park", "airport", "junction", "lake", "beach"
  QString icon;
};

struct Edge {
  int to;
  double distance;
  double trafficFactor;
  QString streetName;
  bool isBridge;
  bool isMajor;
};

struct RouteStep {
  QString from, to;
  double meters;
  int seconds;
  double traffic;
  QColor color;
  QString instruction;
  QString street;
  QString icon;
};

struct Graph {
  QVector<Node> nodes;
  QVector<QVector<Edge>> adj;

  void addNode(int id, const QString &name, QPointF pos,
               const QString &category = "junction", const QString &icon = "📍") {
    nodes.append({id, name, pos, category, icon});
    adj.append(QVector<Edge>());
  }

  void addEdge(int u, int v, double dist, double traffic = 1.0,
               const QString &streetName = "", bool isBridge = false, bool isMajor = true) {
    adj[u].append({v, dist, traffic, streetName, isBridge, isMajor});
    adj[v].append({u, dist, traffic, streetName, isBridge, isMajor});
  }

  static double travelTimeOf(double distPx, double traffic, double v0 = 40.0) {
    double distKm = distPx * 0.005; // 1 pixel = 5 mét trong tỉ lệ bản đồ chuẩn
    double speed = std::max(5.0, v0 / std::max(0.1, traffic));
    return distKm / speed; // giờ
  }

  QVector<int> dijkstra(int start, int end, const QSet<int> &bannedNodes,
                        const QSet<QPair<int, int>> &bannedEdges,
                        double peakMul, double v0) const {
    const int n = nodes.size();
    QVector<double> time(n, std::numeric_limits<double>::max());
    QVector<int> prev(n, -1);
    QVector<bool> visited(n, false);

    if (bannedNodes.contains(start) || bannedNodes.contains(end))
      return {};

    time[start] = 0.0;

    for (int i = 0; i < n; ++i) {
      int u = -1;
      double minT = std::numeric_limits<double>::max();
      for (int j = 0; j < n; ++j) {
        if (!visited[j] && time[j] < minT) {
          minT = time[j];
          u = j;
        }
      }

      if (u == -1 || u == end)
        break;
      visited[u] = true;

      for (const Edge &e : adj[u]) {
        if (bannedNodes.contains(e.to))
          continue;
        QPair<int, int> key1(u, e.to), key2(e.to, u);
        if (bannedEdges.contains(key1) || bannedEdges.contains(key2))
          continue;

        double effectiveTraffic = e.trafficFactor * peakMul;
        double t = travelTimeOf(e.distance, effectiveTraffic, v0);
        if (time[u] + t < time[e.to]) {
          time[e.to] = time[u] + t;
          prev[e.to] = u;
        }
      }
    }

    QVector<int> path;
    if (start == end) {
      path.append(start);
      return path;
    }
    if (time[end] >= std::numeric_limits<double>::max() / 2)
      return path;

    for (int at = end; at != -1; at = prev[at])
      path.prepend(at);
    return path;
  }

  QVector<int> dijkstraByDistance(int start, int end, const QSet<int> &bannedNodes,
                                  const QSet<QPair<int, int>> &bannedEdges) const {
    const int n = nodes.size();
    QVector<double> dist(n, std::numeric_limits<double>::max());
    QVector<int> prev(n, -1);
    QVector<bool> visited(n, false);

    if (bannedNodes.contains(start) || bannedNodes.contains(end))
      return {};

    dist[start] = 0.0;

    for (int i = 0; i < n; ++i) {
      int u = -1;
      double minD = std::numeric_limits<double>::max();
      for (int j = 0; j < n; ++j) {
        if (!visited[j] && dist[j] < minD) {
          minD = dist[j];
          u = j;
        }
      }

      if (u == -1 || u == end)
        break;
      visited[u] = true;

      for (const Edge &e : adj[u]) {
        if (bannedNodes.contains(e.to))
          continue;
        QPair<int, int> key1(u, e.to), key2(e.to, u);
        if (bannedEdges.contains(key1) || bannedEdges.contains(key2))
          continue;

        if (dist[u] + e.distance < dist[e.to]) {
          dist[e.to] = dist[u] + e.distance;
          prev[e.to] = u;
        }
      }
    }

    QVector<int> path;
    if (start == end) {
      path.append(start);
      return path;
    }
    if (dist[end] >= std::numeric_limits<double>::max() / 2)
      return path;

    for (int at = end; at != -1; at = prev[at])
      path.prepend(at);
    return path;
  }
};

// ==================== MAP VIEW (CANVAS & RENDER ĐÀ NẴNG THỰC TẾ) ====================
class MapView : public QGraphicsView {
  Q_OBJECT
public:
  enum RouteMode { TimeRoute = 0, DistRoute = 1 };
  enum TransportMode { Car = 0, Motorbike = 1, Walking = 2 };

  MapView(QWidget *parent = nullptr) : QGraphicsView(parent) {
    scene = new QGraphicsScene(this);
    setScene(scene);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMouseTracking(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setBackgroundBrush(bgColor);

    buildGraph();
    drawMap();
    resetView();

    animTimer = new QTimer(this);
    connect(animTimer, &QTimer::timeout, this, &MapView::onAnimTick);

    pathAnimTimer = new QTimer(this);
    connect(pathAnimTimer, &QTimer::timeout, this, &MapView::onPathAnimTick);

    trafficSimTimer = new QTimer(this);
    connect(trafficSimTimer, &QTimer::timeout, this, &MapView::onTrafficSimTick);

    smoothAnimTimer = new QTimer(this);
    connect(smoothAnimTimer, &QTimer::timeout, this, &MapView::onSmoothAnimTick);

    setupFloatingControls();
    setupFloatingSearchBar();
    setupFloatingRouteBanner();
  }

  // Getters
  const QVector<RouteStep> &lastRoute() const { return lastSteps; }
  double lastTotalMeters() const { return totalMeters; }
  int lastTotalSeconds() const { return totalSeconds; }
  QVector<QVector<QPointF>> lastAlternatives() const { return altPaths; }

  const QVector<RouteStep> &timeRouteSteps() const { return timeSteps; }
  double timeRouteTotalMeters() const { return timeTotalMeters; }
  int timeRouteTotalSeconds() const { return timeTotalSeconds; }
  const QVector<RouteStep> &distRouteSteps() const { return distSteps; }
  double distRouteTotalMeters() const { return distTotalMeters; }
  int distRouteTotalSeconds() const { return distTotalSeconds; }
  bool hasDualRoutes() const { return dualRoutesAvailable; }
  bool areDualRoutesSame() const { return dualRoutesSame; }
  RouteMode currentRouteMode() const { return routeMode; }
  TransportMode currentTransportMode() const { return transportMode; }

  int nodeCount() const { return graph.nodes.size(); }
  int edgeCount() const {
    int c = 0;
    for (int u = 0; u < graph.nodes.size(); ++u)
      for (const Edge &e : graph.adj[u])
        if (u < e.to)
          ++c;
    return c;
  }
  QStringList nodeNames() const {
    QStringList l;
    for (const Node &n : graph.nodes)
      l << QString("%1 (%2)").arg(n.name, n.icon);
    return l;
  }
  int nodeIdByIndex(int idx) const {
    if (idx >= 0 && idx < graph.nodes.size())
      return graph.nodes[idx].id;
    return -1;
  }
  QString nodeNameById(int id) const {
    if (id >= 0 && id < graph.nodes.size())
      return graph.nodes[id].name;
    return "";
  }
  QPointF nodePos(int id) const {
    if (id < 0 || id >= graph.nodes.size())
      return {};
    return graph.nodes[id].pos;
  }
  int nodeDegree(int id) const {
    if (id < 0 || id >= graph.nodes.size())
      return 0;
    return graph.adj[id].size();
  }
  int startNodeId() const { return startEdgeU; }
  int endNodeId() const { return endEdgeU; }

  bool isTrafficLayerVisible() const { return showTrafficLayer; }
  bool isLandmarksLayerVisible() const { return showLandmarksLayer; }
  bool isStreetLabelsLayerVisible() const { return showStreetLabelsLayer; }
  bool isFollowVehicle() const { return followVehicle; }

  // Setters
  void setV0(double v) {
    v0 = v;
    recompute();
  }
  void setPeakMul(double m) {
    peakMul = m;
    recompute();
  }
  void setUseMeters(bool b) {
    useMeters = b;
    emit statusMessage(QString("Đơn vị: %1").arg(b ? "mét" : "km"));
    updateRouteBanner();
    viewport()->update();
  }
  void setDarkMode(bool b) {
    darkMode = b;
    applyTheme();
  }
  void setGridVisible(bool b) {
    gridVisible = b;
    if (gridGroup)
      gridGroup->setVisible(b);
  }
  void setBanLongBridge(bool b) {
    banLongBridge = b;
    // Cầu Rồng giữa cầu là Node 55
    if (b)
      bannedNodes.insert(55);
    else
      bannedNodes.remove(55);
    recompute();
  }
  void setAvoidCongested(bool b) {
    avoidCongested = b;
    recompute();
  }
  void setShowAlternatives(bool b) {
    showAlternatives = b;
    recompute();
  }
  void setRouteMode(int mode) {
    if (routeMode == static_cast<RouteMode>(mode))
      return;
    routeMode = static_cast<RouteMode>(mode);
    recompute();
    emit routeModeChanged(mode);
  }
  void setTransportMode(int mode) {
    transportMode = static_cast<TransportMode>(mode);
    if (transportMode == Car)
      v0 = 40.0;
    else if (transportMode == Motorbike)
      v0 = 35.0;
    else if (transportMode == Walking)
      v0 = 5.0;
    recompute();
  }

  void setTrafficLayerVisible(bool b) {
    showTrafficLayer = b;
    if (legendGroup)
      legendGroup->setVisible(b);
    if (!startPoint.isNull() && !endPoint.isNull()) {
      recompute();
    }
    viewport()->update();
    emit trafficLayerVisibilityChanged(b);
    emit statusMessage(b ? "🚦 Mật độ giao thông trên tuyến đường: BẬT"
                         : "⚪ Mật độ giao thông trên tuyến đường: TẮT (hiển thị màu xanh chuẩn)");
  }

  void toggleTrafficLayer() {
    setTrafficLayerVisible(!showTrafficLayer);
  }

  void setLandmarksLayerVisible(bool b) {
    showLandmarksLayer = b;
    if (poiGroup)
      poiGroup->setVisible(b);
    viewport()->update();
    emit statusMessage(b ? "Đã bật lớp địa danh & POI" : "Đã ẩn lớp địa danh");
  }

  void setStreetLabelsLayerVisible(bool b) {
    showStreetLabelsLayer = b;
    if (streetLabelGroup)
      streetLabelGroup->setVisible(b);
    viewport()->update();
    emit statusMessage(b ? "Đã bật lớp tên đường" : "Đã ẩn lớp tên đường");
  }

  void setFollowVehicle(bool b) {
    followVehicle = b;
    emit statusMessage(b ? "Chế độ camera bám theo xe: BẬT" : "Chế độ camera bám theo xe: TẮT");
  }

  void setEndpoints(int u, int v) {
    if (u < 0 || u >= graph.nodes.size() || v < 0 || v >= graph.nodes.size() || u == v)
      return;
    clearPath();
    clearMarkers();
    stopAnimation();
    stopPathAnim();
    startPoint = graph.nodes[u].pos;
    startEdgeU = u;
    startEdgeV = -1;
    addMarker(startPoint, colorA, "A");
    endPoint = graph.nodes[v].pos;
    endEdgeU = v;
    endEdgeV = -1;
    addMarker(endPoint, colorB, "B");
    recompute();
    emit endpointsChanged(u, v);
    emit routeChanged();
  }

public slots:
  void setTrafficSimulation(bool enabled) {
    trafficSimEnabled = enabled;
    if (trafficSimEnabled) {
      trafficSimTimer->start(1000);
      onTrafficSimTick();
    } else {
      trafficSimTimer->stop();
    }
  }

  void clearAll() {
    clearPath();
    clearMarkers();
    stopAnimation();
    stopPathAnim();
    startPoint = endPoint = QPointF();
    startEdgeU = startEdgeV = endEdgeU = endEdgeV = -1;
    lastSteps.clear();
    totalMeters = 0;
    totalSeconds = 0;
    altPaths.clear();
    if (floatingRouteBanner)
      floatingRouteBanner->hide();
    emit endpointsChanged(-1, -1);
    emit routeChanged();
    emit statusMessage("Đã xoá. Chọn điểm A → B để tìm đường.");
  }

  void swapAB() {
    std::swap(startPoint, endPoint);
    std::swap(startEdgeU, endEdgeU);
    std::swap(startEdgeV, endEdgeV);
    clearMarkers();
    redrawMarkers();
    emit endpointsChanged(startEdgeU, endEdgeU);
    if (!startPoint.isNull() && !endPoint.isNull())
      recompute();
  }

  void zoomIn() { scale(1.2, 1.2); viewport()->update(); }
  void zoomOut() { scale(0.83, 0.83); viewport()->update(); }
  void resetView() {
    resetTransform();
    fitInView(QRectF(15, -10, 1100, 690), Qt::KeepAspectRatio);
    viewport()->update();
  }
  void fitView() {
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    viewport()->update();
  }

  void centerOnNode(int id) {
    if (id < 0 || id >= graph.nodes.size())
      return;
    centerOn(graph.nodes[id].pos);
    auto *flash = scene->addEllipse(
        graph.nodes[id].pos.x() - 20, graph.nodes[id].pos.y() - 20, 40, 40,
        QPen(QColor(66, 133, 244), 3.5), QBrush(QColor(66, 133, 244, 70)));
    flash->setZValue(45);
    QTimer::singleShot(1100, this, [this, flash]() {
      if (scene && flash->scene()) {
        scene->removeItem(flash);
        delete flash;
      }
    });
  }

  void playNavigationSimulation() {
    if (primaryPoints.size() >= 2) {
      startAnimation(primaryPoints);
    }
  }

  void savePng(const QString &fn) {
    QPixmap pix(size());
    pix.fill(darkMode ? QColor(24, 26, 32) : QColor(242, 239, 233));
    QPainter p(&pix);
    render(&p);
    p.end();
    pix.save(fn);
    emit statusMessage("Đã lưu ảnh: " + fn);
  }

signals:
  void routeChanged();
  void statusMessage(const QString &msg);
  void mousePosChanged(QPointF scenePos);
  void routeModeChanged(int mode);
  void endpointsChanged(int startId, int endId);
  void trafficSimTimeChanged(int hour, const QString &desc);
  void trafficLayerVisibilityChanged(bool visible);

protected:
  void resizeEvent(QResizeEvent *e) override {
    QGraphicsView::resizeEvent(e);
    if (floatingSearchBar) {
      floatingSearchBar->move(18, 16);
    }
    if (floatingRouteBanner) {
      int bw = floatingRouteBanner->width();
      floatingRouteBanner->move(std::max(390, width() / 2 - bw / 2), 16);
    }
    if (floatingControlFrame) {
      floatingControlFrame->move(width() - floatingControlFrame->width() - 16,
                                 height() - floatingControlFrame->height() - 24);
    }
  }

  void mouseMoveEvent(QMouseEvent *e) override {
    currentMouseScenePos = mapToScene(e->pos());
    emit mousePosChanged(currentMouseScenePos);

    int oldHoverNode = hoveredNodeId;
    int oldHoverU = hoveredEdgeU;
    int oldHoverV = hoveredEdgeV;

    hoveredNodeId = findNearestNode(currentMouseScenePos, 22.0);
    if (hoveredNodeId < 0) {
      auto [u, v, proj] = findNearestPointOnEdge(currentMouseScenePos);
      if (u != -1 && QLineF(currentMouseScenePos, proj).length() <= 16.0) {
        hoveredEdgeU = u;
        hoveredEdgeV = v;
      } else {
        hoveredEdgeU = hoveredEdgeV = -1;
      }
    } else {
      hoveredEdgeU = hoveredEdgeV = -1;
    }

    if (hoveredNodeId >= 0 || hoveredEdgeU >= 0) {
      setCursor(Qt::PointingHandCursor);
    } else {
      setCursor(Qt::OpenHandCursor);
    }

    if (hoveredNodeId != oldHoverNode || hoveredEdgeU != oldHoverU || hoveredEdgeV != oldHoverV) {
      viewport()->update();
    }

    QGraphicsView::mouseMoveEvent(e);
  }

  void leaveEvent(QEvent *e) override {
    if (hoveredNodeId >= 0 || hoveredEdgeU >= 0) {
      hoveredNodeId = -1;
      hoveredEdgeU = hoveredEdgeV = -1;
      viewport()->update();
    }
    QGraphicsView::leaveEvent(e);
  }

  void mousePressEvent(QMouseEvent *event) override {
    // Kiểm tra nhấp vào La Bàn (Compass) góc trên bên phải
    int vpW = viewport()->width();
    int cx = vpW - 40;
    int cy = 35;
    if (QLineF(event->pos(), QPointF(cx, cy)).length() <= 20.0) {
      resetView();
      emit statusMessage("Đã căn lại hướng Bắc & vị trí trung tâm");
      return;
    }

    if (event->button() == Qt::RightButton) {
      handleRightClick(event);
      return;
    }
    if (event->button() == Qt::LeftButton) {
      QPointF clickPos = mapToScene(event->pos());
      int nearestNode = findNearestNode(clickPos, 20.0);
      if (nearestNode >= 0) {
        setEndpointFromNode(nearestNode);
        return;
      }
      auto [edgeU, edgeV, projected] = findNearestPointOnEdge(clickPos);
      if (edgeU != -1) {
        setEndpointFromEdge(edgeU, edgeV, projected);
        return;
      }
    }
    QGraphicsView::mousePressEvent(event);
  }

  void mouseDoubleClickEvent(QMouseEvent *event) override {
    if (event->button() != Qt::LeftButton)
      return;
    QPointF clickPos = mapToScene(event->pos());
    int nid = findNearestNode(clickPos, 24.0);
    if (nid >= 0)
      setEndpointFromNode(nid);
  }

  void wheelEvent(QWheelEvent *event) override {
    double f = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    scale(f, f);
    viewport()->update();
  }

  void keyPressEvent(QKeyEvent *e) override {
    switch (e->key()) {
    case Qt::Key_Escape:
      clearAll();
      break;
    case Qt::Key_R:
      resetView();
      break;
    case Qt::Key_F:
      fitView();
      break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
      zoomIn();
      break;
    case Qt::Key_Minus:
      zoomOut();
      break;
    default:
      QGraphicsView::keyPressEvent(e);
    }
  }

  void drawForeground(QPainter *painter, const QRectF &) override {
    painter->save();
    painter->setWorldMatrixEnabled(false);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    int vpW = viewport()->width();
    int vpH = viewport()->height();

    // 1. THANH TỈ LỆ THƯỚC ĐO (GOOGLE MAPS SCALE BAR)
    double scaleFactor = transform().m11();
    double desiredMeters = 500.0;
    if (scaleFactor > 2.2) desiredMeters = 200.0;
    if (scaleFactor > 4.5) desiredMeters = 100.0;
    if (scaleFactor < 0.7) desiredMeters = 1000.0;

    double barPx = (desiredMeters / 5.0) * scaleFactor;
    int x0 = 20;
    int y0 = vpH - 24;

    QColor cardBg = darkMode ? QColor(32, 34, 42, 230) : QColor(255, 255, 255, 235);
    QColor barCol = darkMode ? QColor(210, 215, 225) : QColor(60, 64, 67);

    painter->setPen(Qt::NoPen);
    painter->setBrush(cardBg);
    painter->drawRoundedRect(x0 - 8, y0 - 18, barPx + 16, 26, 6, 6);

    painter->setPen(QPen(barCol, 2));
    painter->drawLine(QLineF(x0, y0, x0 + barPx, y0));
    painter->drawLine(QLineF(x0, y0 - 4, x0, y0 + 2));
    painter->drawLine(QLineF(x0 + barPx, y0 - 4, x0 + barPx, y0 + 2));

    QFont sf("Segoe UI", 9, QFont::DemiBold);
    painter->setFont(sf);
    painter->setPen(barCol);
    QString distText = (desiredMeters >= 1000.0)
                           ? QString("%1 km").arg(desiredMeters / 1000.0, 0, 'f', 1)
                           : QString("%1 m").arg(desiredMeters, 0, 'f', 0);
    painter->drawText(QRectF(x0, y0 - 18, barPx, 16), Qt::AlignCenter, distText);

    QFont wf("Segoe UI", 8);
    painter->setFont(wf);
    painter->setPen(darkMode ? QColor(130, 135, 150) : QColor(128, 134, 139));
    painter->drawText(x0 + barPx + 20, y0 + 2, "Bản đồ TP. Đà Nẵng • Google Maps Edition");

    // 2. LA BÀN TƯƠNG TÁC (INTERACTIVE COMPASS)
    int cx = vpW - 40;
    int cy = 35;
    painter->setPen(QPen(darkMode ? QColor(70, 75, 90) : QColor(218, 220, 224), 1.5));
    painter->setBrush(cardBg);
    painter->drawEllipse(QPointF(cx, cy), 18, 18);

    QPolygonF northArrow;
    northArrow << QPointF(cx, cy - 13) << QPointF(cx - 5, cy) << QPointF(cx + 5, cy);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(234, 67, 53));
    painter->drawPolygon(northArrow);

    QPolygonF southArrow;
    southArrow << QPointF(cx, cy + 13) << QPointF(cx - 5, cy) << QPointF(cx + 5, cy);
    painter->setBrush(darkMode ? QColor(150, 155, 165) : QColor(180, 185, 190));
    painter->drawPolygon(southArrow);

    QFont cf("Segoe UI", 8, QFont::Bold);
    painter->setFont(cf);
    painter->setPen(QColor(234, 67, 53));
    painter->drawText(QRectF(cx - 10, cy - 27, 20, 12), Qt::AlignCenter, "N");

    // 3. HOVER TOOLTIP CARD (GOOGLE MAPS INFO CARD KHI RÊ CHUỘT)
    if (hoveredNodeId >= 0 && hoveredNodeId < graph.nodes.size()) {
      const Node &n = graph.nodes[hoveredNodeId];
      QPoint viewPt = mapFromScene(n.pos);

      // Vẽ vòng sáng hào quang quanh điểm hover trong tọa độ viewport
      painter->setPen(QPen(QColor(66, 133, 244, 210), 2.5));
      painter->setBrush(QColor(66, 133, 244, 45));
      painter->drawEllipse(viewPt, 16, 16);

      // Tính toán vị trí thẻ tooltip
      int cardW = 270;
      int cardH = 88;
      int cardX = viewPt.x() + 20;
      int cardY = viewPt.y() - 44;

      if (cardX + cardW > vpW - 16)
        cardX = viewPt.x() - cardW - 20;
      if (cardY < 60)
        cardY = 60;
      if (cardY + cardH > vpH - 40)
        cardY = vpH - cardH - 40;

      // Nền thẻ Card + đổ bóng
      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(0, 0, 0, darkMode ? 80 : 35));
      painter->drawRoundedRect(cardX + 2, cardY + 3, cardW, cardH, 10, 10);

      QColor cardBody = darkMode ? QColor(36, 38, 48, 245) : QColor(255, 255, 255, 248);
      QColor cardBdr  = darkMode ? QColor(70, 75, 95)       : QColor(218, 220, 224);
      painter->setPen(QPen(cardBdr, 1));
      painter->setBrush(cardBody);
      painter->drawRoundedRect(cardX, cardY, cardW, cardH, 10, 10);

      // Icon & Tên địa điểm
      QFont tf("Segoe UI", 10, QFont::Bold);
      painter->setFont(tf);
      painter->setPen(darkMode ? QColor(138, 180, 248) : QColor(26, 115, 232));
      painter->drawText(cardX + 12, cardY + 22, QString("%1  %2").arg(n.icon, n.name));

      // Phân loại địa điểm & số tuyến nối
      QFont subF("Segoe UI", 8);
      painter->setFont(subF);
      painter->setPen(darkMode ? QColor(180, 185, 195) : QColor(95, 99, 104));
      QString catStr = "Nút giao thông";
      if (n.category == "airport") catStr = "✈️ Cảng hàng không quốc tế";
      else if (n.category == "bridge") catStr = "🌉 Cầu vượt sông Hàn";
      else if (n.category == "park") catStr = "🌳 Công viên & Sinh thái";
      else if (n.category == "poi") catStr = "🛍️ Điểm đến nổi tiếng";
      else if (n.category == "station") catStr = "🚆 Ga đường sắt";
      else if (n.category == "lake") catStr = "💧 Hồ cảnh quan";
      else if (n.category == "beach") catStr = "🏖️ Bãi biển du lịch";

      painter->drawText(cardX + 12, cardY + 40, QString("%1 • %2 nhánh kết nối").arg(catStr).arg(graph.adj[n.id].size()));

      // Dòng hướng dẫn thao tác
      painter->setPen(darkMode ? QColor(150, 155, 165) : QColor(128, 134, 139));
      painter->drawText(cardX + 12, cardY + 58, "👉 Click chuột trái để chọn điểm đi / đến");

      // Trạng thái mật độ khu vực
      double avgT = 1.0;
      int edgeCnt = graph.adj[n.id].size();
      if (edgeCnt > 0) {
        double sum = 0;
        for (const auto &ed : graph.adj[n.id]) sum += ed.trafficFactor;
        avgT = sum / edgeCnt;
      }
      QColor chipC = trafficToColor(avgT);
      painter->setPen(Qt::NoPen);
      painter->setBrush(chipC);
      painter->drawEllipse(cardX + 14, cardY + 71, 7, 7);

      painter->setPen(chipC);
      QFont cf2("Segoe UI", 8, QFont::DemiBold);
      painter->setFont(cf2);
      QString tStatus = (avgT * peakMul <= 1.2) ? "Lưu thông thông thoáng" : ((avgT * peakMul <= 1.8) ? "Mật độ trung bình" : "Đang đông / ùn ứ");
      painter->drawText(cardX + 26, cardY + 78, tStatus);

    } else if (hoveredEdgeU >= 0 && hoveredEdgeV >= 0) {
      // Hover trên một đoạn đường
      double traf = trafficOf(graph, hoveredEdgeU, hoveredEdgeV);
      QString street = streetOf(graph, hoveredEdgeU, hoveredEdgeV);
      if (street.isEmpty())
        street = QString("%1 ↔ %2").arg(graph.nodes[hoveredEdgeU].name, graph.nodes[hoveredEdgeV].name);

      QPoint viewPt = mapFromScene(currentMouseScenePos);
      int cardW = 240;
      int cardH = 56;
      int cardX = viewPt.x() + 18;
      int cardY = viewPt.y() - 28;

      if (cardX + cardW > vpW - 16)
        cardX = viewPt.x() - cardW - 18;
      if (cardY < 60)
        cardY = 60;
      if (cardY + cardH > vpH - 40)
        cardY = vpH - cardH - 40;

      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(0, 0, 0, darkMode ? 80 : 35));
      painter->drawRoundedRect(cardX + 2, cardY + 3, cardW, cardH, 8, 8);

      QColor cardBody = darkMode ? QColor(36, 38, 48, 245) : QColor(255, 255, 255, 248);
      QColor cardBdr  = darkMode ? QColor(70, 75, 95)       : QColor(218, 220, 224);
      painter->setPen(QPen(cardBdr, 1));
      painter->setBrush(cardBody);
      painter->drawRoundedRect(cardX, cardY, cardW, cardH, 8, 8);

      QFont tf("Segoe UI", 9, QFont::Bold);
      painter->setFont(tf);
      painter->setPen(darkMode ? QColor(230, 235, 245) : QColor(32, 33, 36));
      painter->drawText(cardX + 10, cardY + 20, "🛣️ " + street);

      QColor c = trafficToColor(traf);
      painter->setBrush(c);
      painter->drawEllipse(cardX + 12, cardY + 33, 6, 6);

      QFont sf2("Segoe UI", 8);
      painter->setFont(sf2);
      painter->setPen(c);
      QString tMsg = QString("Mật độ: %1 • Tốc độ ~%2 km/h")
                         .arg(traf * peakMul, 0, 'f', 1)
                         .arg(std::max(5.0, v0 / std::max(0.1, traf * peakMul)), 0, 'f', 0);
      painter->drawText(cardX + 24, cardY + 40, tMsg);
    }

    painter->restore();
  }

private:
  QGraphicsScene *scene = nullptr;
  Graph graph;

  QPointF startPoint, endPoint;
  int startEdgeU = -1, startEdgeV = -1;
  int endEdgeU = -1, endEdgeV = -1;

  QVector<QGraphicsItem *> pathItems;
  QVector<QGraphicsItem *> markerItems;
  QVector<QGraphicsItem *> altPathItems;
  QVector<QGraphicsItem *> secondaryPathItems;
  QGraphicsItemGroup *gridGroup = nullptr;
  QGraphicsItemGroup *poiGroup = nullptr;
  QGraphicsItemGroup *streetLabelGroup = nullptr;
  QGraphicsItemGroup *legendGroup = nullptr;
  QToolButton *btnTraffic = nullptr;

  bool showTrafficLayer = true;
  bool showLandmarksLayer = true;
  bool showStreetLabelsLayer = true;
  bool followVehicle = false;

  int hoveredNodeId = -1;
  int hoveredEdgeU = -1, hoveredEdgeV = -1;
  QPointF currentMouseScenePos;

  QVector<RouteStep> lastSteps;
  QVector<QVector<QPointF>> altPaths;
  double totalMeters = 0;
  int totalSeconds = 0;

  RouteMode routeMode = TimeRoute;
  TransportMode transportMode = Car;
  bool dualRoutesAvailable = false;
  bool dualRoutesSame = false;
  QVector<RouteStep> timeSteps, distSteps;
  double timeTotalMeters = 0, distTotalMeters = 0;
  int timeTotalSeconds = 0, distTotalSeconds = 0;

  double v0 = 40.0;
  double peakMul = 1.0;
  bool useMeters = true;
  bool darkMode = false;
  bool gridVisible = false;
  bool banLongBridge = false;
  bool avoidCongested = false;
  bool showAlternatives = false;
  QSet<int> bannedNodes;
  QSet<QPair<int, int>> bannedEdges;

  QColor colorA = QColor(52, 168, 83);     // Marker A (Green)
  QColor colorB = QColor(234, 67, 53);     // Marker B (Red)
  QColor bgColor = QColor(242, 239, 233);  // Google Maps Land
  QColor roadColor = QColor(196, 190, 176); // Road outline casing
  QColor textColor = QColor(60, 64, 67);   // Label text

  QTimer *animTimer = nullptr;
  QGraphicsItemGroup *mover = nullptr;
  QVector<QPointF> animPath;
  int animIndex = 0;

  // Smooth vehicle simulation
  QTimer *smoothAnimTimer = nullptr;
  struct AnimSeg {
    QPointF p0, p1;
    double len;
    double cumDist;
    double angleDeg;
  };
  QVector<AnimSeg> animSegments;
  double animTotalDist = 0.0;
  double animCurrentDist = 0.0;
  double animSpeed = 180.0; // px/sec

  QTimer *pathAnimTimer = nullptr;
  QVector<QGraphicsLineItem *> pathAnimLines;
  int pathAnimIndex = 0;
  QVector<QPointF> primaryPoints;

  QTimer *trafficSimTimer = nullptr;
  bool trafficSimEnabled = false;
  int simHour = 8; // giờ giả lập hiện tại (6-22)
  int simMinute = 0; // phút giả lập (0-59)

  // Floating Widgets
  QFrame *floatingControlFrame = nullptr;
  QFrame *floatingSearchBar = nullptr;
  QLineEdit *searchInput = nullptr;
  QToolButton *searchClearBtn = nullptr;
  QCompleter *searchCompleter = nullptr;

  QFrame *floatingRouteBanner = nullptr;
  QLabel *bannerModeIcon = nullptr;
  QLabel *bannerEtaLabel = nullptr;
  QLabel *bannerDistLabel = nullptr;
  QLabel *bannerDetailLabel = nullptr;
  QPushButton *bannerSimBtn = nullptr;

  void setupFloatingControls() {
    floatingControlFrame = new QFrame(this);
    floatingControlFrame->setObjectName("floatingControls");

    QVBoxLayout *flay = new QVBoxLayout(floatingControlFrame);
    flay->setContentsMargins(4, 4, 4, 4);
    flay->setSpacing(3);

    auto makeBtn = [this, flay](const QString &text, const QString &tip, auto slot) {
      QToolButton *btn = new QToolButton(floatingControlFrame);
      btn->setText(text);
      btn->setToolTip(tip);
      connect(btn, &QToolButton::clicked, this, slot);
      flay->addWidget(btn);
      return btn;
    };

    makeBtn("+", "Phóng to (Zoom In)", &MapView::zoomIn);
    makeBtn("−", "Thu nhỏ (Zoom Out)", &MapView::zoomOut);

    QFrame *sep = new QFrame(floatingControlFrame);
    sep->setFrameShape(QFrame::HLine);
    flay->addWidget(sep);

    makeBtn("⌖", "Về trung tâm Đà Nẵng", &MapView::resetView);
    makeBtn("⛶", "Vừa toàn cảnh (Fit)", &MapView::fitView);

    QFrame *sep2 = new QFrame(floatingControlFrame);
    sep2->setFrameShape(QFrame::HLine);
    flay->addWidget(sep2);

    QToolButton *btnFollow = new QToolButton(floatingControlFrame);
    btnFollow->setText("🎯");
    btnFollow->setToolTip("Bám theo xe khi dẫn đường (Follow Camera)");
    btnFollow->setCheckable(true);
    btnFollow->setChecked(followVehicle);
    connect(btnFollow, &QToolButton::toggled, this, &MapView::setFollowVehicle);
    flay->addWidget(btnFollow);

    btnTraffic = new QToolButton(floatingControlFrame);
    btnTraffic->setText("🚦");
    btnTraffic->setToolTip("Bật/Tắt màu sắc mật độ giao thông trên tuyến đường (A → B)");
    btnTraffic->setCheckable(true);
    btnTraffic->setChecked(showTrafficLayer);
    connect(btnTraffic, &QToolButton::toggled, this, &MapView::setTrafficLayerVisible);
    flay->addWidget(btnTraffic);

    connect(this, &MapView::trafficLayerVisibilityChanged, btnTraffic, [this](bool v) {
      if (btnTraffic && btnTraffic->isChecked() != v) {
        btnTraffic->blockSignals(true);
        btnTraffic->setChecked(v);
        btnTraffic->blockSignals(false);
      }
    });

    floatingControlFrame->adjustSize();
    updateFloatingControlTheme();
  }

  void updateFloatingControlTheme() {
    if (!floatingControlFrame) return;
    if (darkMode) {
      floatingControlFrame->setStyleSheet(R"(
        QFrame#floatingControls {
          background: #282A36;
          border: 1px solid #44475A;
          border-radius: 8px;
        }
        QToolButton {
          background: transparent;
          border: none;
          border-radius: 4px;
          color: #F8F8F2;
          font-family: 'Segoe UI';
          font-size: 14px;
          font-weight: bold;
          min-width: 32px;
          min-height: 32px;
        }
        QToolButton:hover { background: #44475A; }
        QToolButton:pressed, QToolButton:checked { background: #6272A4; }
      )");
    } else {
      floatingControlFrame->setStyleSheet(R"(
        QFrame#floatingControls {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 8px;
        }
        QToolButton {
          background: transparent;
          border: none;
          border-radius: 4px;
          color: #3C4043;
          font-family: 'Segoe UI';
          font-size: 14px;
          font-weight: bold;
          min-width: 32px;
          min-height: 32px;
        }
        QToolButton:hover { background: #F1F3F4; }
        QToolButton:pressed, QToolButton:checked { background: #E8F0FE; color: #1A73E8; }
      )");
    }
  }

  void setupFloatingSearchBar() {
    floatingSearchBar = new QFrame(this);
    floatingSearchBar->setObjectName("searchBarFrame");

    QHBoxLayout *lay = new QHBoxLayout(floatingSearchBar);
    lay->setContentsMargins(10, 4, 8, 4);
    lay->setSpacing(6);

    QLabel *icon = new QLabel("🔍", floatingSearchBar);
    icon->setStyleSheet("font-size: 14px; background: transparent;");
    lay->addWidget(icon);

    searchInput = new QLineEdit(floatingSearchBar);
    searchInput->setPlaceholderText("Tìm kiếm địa điểm, cầu, đường phố Đà Nẵng...");
    searchInput->setFrame(false);
    lay->addWidget(searchInput, 1);

    searchClearBtn = new QToolButton(floatingSearchBar);
    searchClearBtn->setText("✕");
    searchClearBtn->setToolTip("Xoá tìm kiếm");
    searchClearBtn->setVisible(false);
    connect(searchClearBtn, &QToolButton::clicked, this, [this]() {
      searchInput->clear();
      searchClearBtn->setVisible(false);
    });
    connect(searchInput, &QLineEdit::textChanged, this, [this](const QString &t) {
      searchClearBtn->setVisible(!t.isEmpty());
    });
    lay->addWidget(searchClearBtn);

    QToolButton *layersBtn = new QToolButton(floatingSearchBar);
    layersBtn->setText("🥞 Lớp");
    layersBtn->setToolTip("Tùy chọn lớp hiển thị bản đồ");
    QMenu *layersMenu = new QMenu(layersBtn);

    QAction *actTraffic = layersMenu->addAction("🚦 Mật độ giao thông (Tuyến đường)");
    actTraffic->setCheckable(true);
    actTraffic->setChecked(showTrafficLayer);
    connect(actTraffic, &QAction::toggled, this, &MapView::setTrafficLayerVisible);
    connect(this, &MapView::trafficLayerVisibilityChanged, actTraffic, [actTraffic](bool v) {
      actTraffic->setChecked(v);
    });

    QAction *actPOI = layersMenu->addAction("📍 Địa danh & POI");
    actPOI->setCheckable(true);
    actPOI->setChecked(showLandmarksLayer);
    connect(actPOI, &QAction::toggled, this, &MapView::setLandmarksLayerVisible);

    QAction *actStreet = layersMenu->addAction("🏷️ Tên đường phố");
    actStreet->setCheckable(true);
    actStreet->setChecked(showStreetLabelsLayer);
    connect(actStreet, &QAction::toggled, this, &MapView::setStreetLabelsLayerVisible);

    layersMenu->addSeparator();
    QAction *actFollow = layersMenu->addAction("🚗 Bám theo xe khi dẫn đường");
    actFollow->setCheckable(true);
    actFollow->setChecked(followVehicle);
    connect(actFollow, &QAction::toggled, this, &MapView::setFollowVehicle);

    layersBtn->setMenu(layersMenu);
    layersBtn->setPopupMode(QToolButton::InstantPopup);
    lay->addWidget(layersBtn);

    QStringList searchPool;
    for (int i = 0; i < graph.nodes.size(); ++i) {
      searchPool << QString("%1 (%2)").arg(graph.nodes[i].name, graph.nodes[i].icon);
    }
    searchCompleter = new QCompleter(searchPool, searchInput);
    searchCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    searchCompleter->setFilterMode(Qt::MatchContains);
    searchInput->setCompleter(searchCompleter);

    auto onSelectMatch = [this](const QString &text) {
      for (int i = 0; i < graph.nodes.size(); ++i) {
        if (text.contains(graph.nodes[i].name) || graph.nodes[i].name.contains(text.trimmed())) {
          centerOnNode(i);
          emit statusMessage(QString("📍 Đã tìm thấy: %1").arg(graph.nodes[i].name));
          break;
        }
      }
    };
    connect(searchCompleter, QOverload<const QString &>::of(&QCompleter::activated), this, onSelectMatch);
    connect(searchInput, &QLineEdit::returnPressed, this, [this, onSelectMatch]() {
      onSelectMatch(searchInput->text());
    });

    floatingSearchBar->setFixedSize(360, 42);
    updateFloatingSearchBarTheme();
  }

  void updateFloatingSearchBarTheme() {
    if (!floatingSearchBar) return;
    if (darkMode) {
      floatingSearchBar->setStyleSheet(R"(
        QFrame#searchBarFrame {
          background: #2D3038;
          border: 1px solid #44475A;
          border-radius: 21px;
        }
        QLineEdit {
          background: transparent;
          color: #E8EAED;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QToolButton {
          background: transparent;
          border: none;
          color: #8AB4F8;
          font-weight: bold;
          font-size: 12px;
          padding: 4px 8px;
          border-radius: 12px;
        }
        QToolButton:hover { background: #3C4048; }
      )");
    } else {
      floatingSearchBar->setStyleSheet(R"(
        QFrame#searchBarFrame {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 21px;
        }
        QLineEdit {
          background: transparent;
          color: #202124;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QToolButton {
          background: transparent;
          border: none;
          color: #1A73E8;
          font-weight: bold;
          font-size: 12px;
          padding: 4px 8px;
          border-radius: 12px;
        }
        QToolButton:hover { background: #F1F3F4; }
      )");
    }
  }

  void setupFloatingRouteBanner() {
    floatingRouteBanner = new QFrame(this);
    floatingRouteBanner->setObjectName("routeBannerFrame");

    QHBoxLayout *lay = new QHBoxLayout(floatingRouteBanner);
    lay->setContentsMargins(14, 4, 10, 4);
    lay->setSpacing(8);

    bannerModeIcon = new QLabel("🚗", floatingRouteBanner);
    bannerModeIcon->setStyleSheet("font-size: 18px; background: transparent;");
    lay->addWidget(bannerModeIcon);

    bannerEtaLabel = new QLabel("-- phút", floatingRouteBanner);
    bannerEtaLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #188038; background: transparent;");
    lay->addWidget(bannerEtaLabel);

    bannerDistLabel = new QLabel("(-- km)", floatingRouteBanner);
    bannerDistLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #5F6368; background: transparent;");
    lay->addWidget(bannerDistLabel);

    QFrame *vsep = new QFrame(floatingRouteBanner);
    vsep->setFrameShape(QFrame::VLine);
    vsep->setStyleSheet("border-color: #DADCE0;");
    lay->addWidget(vsep);

    bannerDetailLabel = new QLabel("Tuyến tối ưu nhất", floatingRouteBanner);
    bannerDetailLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #1A73E8; background: #E8F0FE; padding: 2px 8px; border-radius: 10px;");
    lay->addWidget(bannerDetailLabel);

    bannerSimBtn = new QPushButton("▶ Mô phỏng", floatingRouteBanner);
    connect(bannerSimBtn, &QPushButton::clicked, this, &MapView::playNavigationSimulation);
    lay->addWidget(bannerSimBtn);

    QToolButton *closeBtn = new QToolButton(floatingRouteBanner);
    closeBtn->setText("✕");
    closeBtn->setStyleSheet("background: transparent; border: none; font-size: 12px; color: #5F6368;");
    connect(closeBtn, &QToolButton::clicked, floatingRouteBanner, &QFrame::hide);
    lay->addWidget(closeBtn);

    floatingRouteBanner->setFixedHeight(42);
    floatingRouteBanner->hide();
    updateFloatingRouteBannerTheme();
  }

  void updateFloatingRouteBannerTheme() {
    if (!floatingRouteBanner) return;
    if (darkMode) {
      floatingRouteBanner->setStyleSheet(R"(
        QFrame#routeBannerFrame {
          background: #2D3038;
          border: 1px solid #44475A;
          border-radius: 21px;
        }
        QPushButton {
          background: #8AB4F8; color: #202124; border: none; border-radius: 12px;
          padding: 4px 10px; font-weight: bold; font-size: 11px;
        }
        QPushButton:hover { background: #AECBFA; }
      )");
      if (bannerDistLabel)
        bannerDistLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #BDC1C6; background: transparent;");
      if (bannerDetailLabel)
        bannerDetailLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #8AB4F8; background: #384252; padding: 2px 8px; border-radius: 10px;");
    } else {
      floatingRouteBanner->setStyleSheet(R"(
        QFrame#routeBannerFrame {
          background: #FFFFFF;
          border: 1px solid #DADCE0;
          border-radius: 21px;
        }
        QPushButton {
          background: #1A73E8; color: white; border: none; border-radius: 12px;
          padding: 4px 10px; font-weight: bold; font-size: 11px;
        }
        QPushButton:hover { background: #1557B0; }
      )");
      if (bannerDistLabel)
        bannerDistLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #5F6368; background: transparent;");
      if (bannerDetailLabel)
        bannerDetailLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #1A73E8; background: #E8F0FE; padding: 2px 8px; border-radius: 10px;");
    }
  }

  void updateRouteBanner() {
    if (!floatingRouteBanner) return;
    if (lastSteps.isEmpty() || totalMeters <= 0) {
      floatingRouteBanner->hide();
      return;
    }
    QString modeIcon = "🚗";
    if (transportMode == Motorbike) modeIcon = "🛵";
    else if (transportMode == Walking) modeIcon = "🚶‍♂️";
    bannerModeIcon->setText(modeIcon);

    int m = totalSeconds / 60;
    int s = totalSeconds % 60;
    QString etaText = (m > 0) ? QString("%1 ph %2 s").arg(m).arg(s) : QString("%1 giây").arg(s);
    bannerEtaLabel->setText(etaText);
    bannerDistLabel->setText(QString("(%1)").arg(fmtDist(totalMeters)));

    QString detail = (routeMode == DistRoute) ? "Quãng đường ngắn nhất" : "Nhanh nhất tránh kẹt";
    bannerDetailLabel->setText(detail);

    floatingRouteBanner->adjustSize();
    int bw = floatingRouteBanner->width();
    floatingRouteBanner->move(std::max(390, width() / 2 - bw / 2), 16);
    floatingRouteBanner->show();
  }

  void onSmoothAnimTick() {
    if (animSegments.isEmpty()) {
      smoothAnimTimer->stop();
      return;
    }

    animCurrentDist += animSpeed * 0.035;
    if (animCurrentDist >= animTotalDist) {
      animCurrentDist = 0.0; // Vòng lặp mô phỏng liên tục
    }

    // Tìm đoạn đường đang chạy
    const AnimSeg *curSeg = &animSegments.last();
    double segStartDist = 0.0;
    for (const auto &seg : animSegments) {
      if (animCurrentDist <= seg.cumDist) {
        curSeg = &seg;
        segStartDist = seg.cumDist - seg.len;
        break;
      }
    }

    double localT = 0.0;
    if (curSeg->len > 1e-4) {
      localT = std::clamp((animCurrentDist - segStartDist) / curSeg->len, 0.0, 1.0);
    }
    QPointF curPos = curSeg->p0 + localT * (curSeg->p1 - curSeg->p0);

    if (mover && mover->scene()) {
      mover->setPos(curPos);
      mover->setRotation(curSeg->angleDeg + 90.0);
      if (followVehicle) {
        centerOn(curPos);
      }
    }
  }

  QColor trafficToColor(double factor) const {
    double f = factor * peakMul;
    if (f <= 1.25)
      return QColor(15, 157, 88);    // Xanh lá Google Maps (thông thoáng) #0F9D58
    if (f <= 1.55)
      return QColor(251, 188, 4);    // Vàng Google Maps (đông nhẹ) #FBBC04
    if (f <= 1.9)
      return QColor(255, 112, 67);   // Cam (đông vừa) #FF7043
    if (f <= 2.3)
      return QColor(234, 67, 53);    // Đỏ Google Maps (ùn tắc) #EA4335
    return QColor(183, 28, 28);      // Đỏ sẫm (tắc nghẽn nghiêm trọng) #B71C1C
  }

  // ==================== KHỞI TẠO TOÀN BỘ MẠNG LƯỚI ĐƯỜNG ĐÀ NẴNG THỰC TẾ ====================
  void buildGraph() {
    // 1. KHU VỰC TÂY BẮC & THANH KHÊ
    graph.addNode(0, "Nguyễn Tất Thành - Cửa Ngõ Tây", QPointF(-20, 75), "junction", "🌊");
    graph.addNode(1, "Nguyễn Tất Thành - Trần Cao Vân Bắc", QPointF(260, 36), "junction", "🌊");
    graph.addNode(2, "Trần Cao Vân - Thái Thị Bôi", QPointF(120, 130), "junction", "📍");
    graph.addNode(3, "Điện Biên Phủ - Cửa Ngõ Tây (Ngã Ba Huế)", QPointF(-20, 270), "junction", "📍");
    graph.addNode(4, "Điện Biên Phủ - AEON Mall Thanh Khê", QPointF(180, 270), "poi", "🛍️");
    graph.addNode(5, "Cổng Cảng Hàng Không QT Đà Nẵng", QPointF(210, 440), "airport", "✈️");
    graph.addNode(6, "Đường Băng Sân Bay Đà Nẵng", QPointF(95, 520), "airport", "🛫");
    graph.addNode(7, "Điện Biên Phủ - Nguyễn Tri Phương", QPointF(290, 275), "junction", "📍");
    graph.addNode(8, "Hồ Công Viên 29/3 (Bắc)", QPointF(265, 310), "park", "🌳");
    graph.addNode(9, "Hồ Công Viên 29/3 (Nam)", QPointF(265, 380), "park", "🌳");
    graph.addNode(10, "Lê Duy Đình - Nguyễn Tri Phương", QPointF(255, 345), "junction", "📍");
    graph.addNode(11, "Nguyễn Tri Phương - Nguyễn Văn Linh", QPointF(290, 440), "junction", "📍");
    graph.addNode(12, "Nguyễn Hữu Thọ - Duy Tân", QPointF(340, 610), "junction", "📍");
    graph.addNode(13, "Sân Bay Phía Nam Duy Tân", QPointF(230, 610), "airport", "✈️");

    // 2. KHU VỰC TRUNG TÂM (HẢI CHÂU, THẠC GIÁN, VĨNH TRUNG)
    graph.addNode(14, "Lý Thái Tổ - Ga Đà Nẵng", QPointF(360, 275), "station", "🚆");
    graph.addNode(15, "Hải Phòng - Ông Ích Khiêm", QPointF(450, 175), "junction", "📍");
    graph.addNode(16, "Trần Cao Vân - Ông Ích Khiêm", QPointF(440, 125), "junction", "📍");
    graph.addNode(17, "Nguyễn Tất Thành - Ông Ích Khiêm", QPointF(440, 22), "junction", "🌊");
    graph.addNode(18, "Hồ Thạc Gián (Đ. Phan Thanh)", QPointF(360, 330), "lake", "💧");
    graph.addNode(19, "Hồ Hàm Nghi (Đ. Hàm Nghi)", QPointF(410, 330), "lake", "💧");
    graph.addNode(20, "Phan Thanh - Nguyễn Văn Linh", QPointF(360, 440), "junction", "📍");
    graph.addNode(21, "Hàm Nghi - Nguyễn Văn Linh", QPointF(410, 440), "junction", "📍");
    graph.addNode(22, "Lê Đình Lý - Nguyễn Văn Linh", QPointF(450, 440), "junction", "📍");
    graph.addNode(23, "Lê Đình Lý - Duy Tân", QPointF(460, 610), "junction", "📍");
    graph.addNode(24, "Chợ Cồn (Hùng Vương - Ông Ích Khiêm)", QPointF(450, 275), "poi", "🛍️");
    graph.addNode(25, "Ngã Tư Lê Duẩn - Ông Ích Khiêm", QPointF(450, 220), "junction", "📍");
    graph.addNode(26, "Ngã Tư Lê Duẩn - Hoàng Diệu", QPointF(490, 220), "junction", "📍");
    graph.addNode(27, "Hùng Vương - Ngô Gia Tự", QPointF(490, 275), "junction", "📍");
    graph.addNode(28, "Hoàng Diệu - Hoàng Văn Thụ", QPointF(490, 365), "junction", "📍");
    graph.addNode(29, "Hoàng Diệu - Nguyễn Văn Linh", QPointF(490, 440), "junction", "📍");
    graph.addNode(30, "Hoàng Diệu - Duy Tân", QPointF(510, 610), "junction", "📍");
    graph.addNode(31, "Triệu Nữ Vương - Nguyễn Văn Linh", QPointF(470, 440), "junction", "📍");
    graph.addNode(32, "Lê Lợi - Quang Trung", QPointF(530, 80), "junction", "📍");
    graph.addNode(33, "Lê Lợi - Hải Phòng", QPointF(530, 170), "junction", "📍");
    graph.addNode(34, "Ngã Tư Lê Duẩn - Lê Lợi", QPointF(530, 220), "junction", "📍");
    graph.addNode(35, "Phan Châu Trinh - Hùng Vương", QPointF(530, 275), "junction", "📍");
    graph.addNode(36, "Phan Châu Trinh - Hoàng Văn Thụ", QPointF(530, 365), "junction", "📍");
    graph.addNode(37, "Phan Châu Trinh - Nguyễn Văn Linh", QPointF(530, 440), "junction", "📍");
    graph.addNode(38, "Phan Châu Trinh - Trưng Nữ Vương", QPointF(540, 540), "junction", "📍");
    graph.addNode(39, "Trần Phú - Quang Trung", QPointF(610, 80), "junction", "📍");
    graph.addNode(40, "Trần Phú - Hải Phòng", QPointF(610, 165), "junction", "📍");
    graph.addNode(41, "Trần Phú - Lê Duẩn (Tây Cầu Sông Hàn)", QPointF(610, 220), "junction", "📍");
    graph.addNode(42, "Chợ Hàn (Trần Phú - Hùng Vương)", QPointF(610, 275), "poi", "🛍️");
    graph.addNode(43, "Giáo Xứ Chính Tòa (Nhà Thờ Con Gà)", QPointF(610, 320), "poi", "⛪");
    graph.addNode(44, "Trần Phú - Hoàng Văn Thụ", QPointF(610, 365), "junction", "📍");
    graph.addNode(45, "Trần Phú - Nguyễn Văn Linh", QPointF(610, 440), "junction", "📍");
    graph.addNode(46, "Bạch Đằng - Như Nguyệt / Quang Trung", QPointF(660, 80), "junction", "📍");
    graph.addNode(47, "Đầu Cầu Sông Hàn (Bờ Tây Bạch Đằng)", QPointF(660, 150), "bridge", "🌉");
    graph.addNode(48, "Bạch Đằng - Hùng Vương (Bến Du Thuyền)", QPointF(660, 275), "junction", "⚓");
    graph.addNode(49, "Đầu Cầu Rồng (Bờ Tây Bạch Đằng)", QPointF(660, 380), "bridge", "🌉");
    graph.addNode(50, "Công Viên APEC (2 Tháng 9)", QPointF(625, 410), "park", "🌳");
    graph.addNode(51, "Đường 2 Tháng 9 - Bình Minh 1", QPointF(625, 490), "junction", "📍");
    graph.addNode(52, "Đầu Cầu Trần Thị Lý (Bờ Tây 2/9)", QPointF(635, 610), "bridge", "🌉");
    graph.addNode(53, "Hòa Cường (Đường 2/9 - Duy Tân)", QPointF(580, 610), "junction", "📍");

    // 3. CÁC ĐIỂM TRÊN CẦU QUA SÔNG HÀN
    graph.addNode(54, "Cầu Sông Hàn (Giữa Cầu Quay)", QPointF(717, 150), "bridge", "🌉");
    graph.addNode(55, "Cầu Rồng (Vòm Rồng Vàng)", QPointF(717, 380), "bridge", "🐉");
    graph.addNode(56, "Cầu Trần Thị Lý (Trụ Dây Văng)", QPointF(705, 610), "bridge", "🌉");

    // 4. KHU VỰC BỜ ĐÔNG (SƠN TRÀ & NGŨ HÀNH SƠN, BÃI BIỂN MỸ KHÊ)
    graph.addNode(57, "Đầu Cầu Sông Hàn (Bờ Đông Trần Hưng Đạo)", QPointF(775, 150), "bridge", "🌉");
    graph.addNode(58, "Vincom Plaza Đà Nẵng (Sơn Trà)", QPointF(800, 150), "poi", "🛍️");
    graph.addNode(59, "Trần Hưng Đạo - Cầu Sông Hàn Bắc", QPointF(775, 80), "junction", "📍");
    graph.addNode(60, "Đầu Cầu Rồng (Bờ Đông Trần Hưng Đạo)", QPointF(775, 380), "bridge", "🌉");
    graph.addNode(61, "Trần Hưng Đạo - Cầu Rồng Bắc", QPointF(775, 275), "junction", "📍");
    graph.addNode(62, "Trần Hưng Đạo - Cầu Trần Thị Lý", QPointF(775, 610), "junction", "📍");
    graph.addNode(63, "Ngô Quyền - Phạm Văn Đồng (AH17)", QPointF(830, 150), "junction", "📍");
    graph.addNode(64, "Ngô Quyền - Võ Văn Kiệt (AH17)", QPointF(830, 380), "junction", "📍");
    graph.addNode(65, "Ngô Quyền - Trần Thị Lý (AH17)", QPointF(830, 610), "junction", "📍");
    graph.addNode(66, "Phạm Cự Lượng - Võ Văn Kiệt", QPointF(890, 380), "junction", "📍");
    graph.addNode(67, "Phạm Cự Lượng - An Hải", QPointF(890, 270), "junction", "📍");
    graph.addNode(68, "Nguyễn Thiện Kế - Võ Văn Kiệt", QPointF(940, 380), "junction", "📍");
    graph.addNode(69, "Nguyễn Thiện Kế - Lê Hữu Trác", QPointF(940, 460), "junction", "📍");
    graph.addNode(70, "Hồ Nghinh - Phạm Văn Đồng", QPointF(1010, 150), "junction", "📍");
    graph.addNode(71, "Hồ Nghinh - Võ Văn Kiệt", QPointF(1010, 380), "junction", "📍");
    graph.addNode(72, "Hà Bổng - Võ Văn Kiệt", QPointF(1045, 380), "junction", "📍");
    graph.addNode(73, "Hà Bổng - Lê Hữu Trác", QPointF(1045, 460), "junction", "📍");
    graph.addNode(74, "Bãi Tắm Phạm Văn Đồng", QPointF(1085, 150), "beach", "🏖️");
    graph.addNode(75, "Bãi Biển Mỹ Khê (Võ Văn Kiệt)", QPointF(1085, 380), "beach", "🏖️");
    graph.addNode(76, "Võ Nguyên Giáp - Lê Hữu Trác", QPointF(1085, 460), "beach", "🏖️");
    graph.addNode(77, "Võ Nguyên Giáp - Nguyễn Văn Thoại", QPointF(1085, 610), "beach", "🏖️");
    graph.addNode(78, "Trần Thị Lý Đông - Ngũ Hành Sơn", QPointF(830, 640), "junction", "📍");
    graph.addNode(79, "Đỗ Bá - Ngũ Hành Sơn", QPointF(930, 640), "junction", "📍");
    graph.addNode(80, "Khu Đô Thị Bắc Mỹ Phú", QPointF(1010, 640), "poi", "🏘️");

    // 5. CÁC NÚT MỞ RỘNG TẠI THANH KHÊ & ĐƯỜNG NGUYỄN TẤT THÀNH
    graph.addNode(81, "Điện Biên Phủ - Hà Huy Tập", QPointF(60, 270), "junction", "📍");
    graph.addNode(82, "Điện Biên Phủ - Thái Thị Bôi", QPointF(120, 270), "junction", "📍");
    graph.addNode(83, "Điện Biên Phủ - Lê Duy Đình", QPointF(240, 270), "junction", "📍");
    graph.addNode(84, "Nguyễn Tất Thành - Hà Huy Tập", QPointF(60, 60), "junction", "🌊");
    graph.addNode(85, "Nguyễn Tất Thành - Thái Thị Bôi Bắc", QPointF(160, 48), "junction", "🌊");
    graph.addNode(86, "Nguyễn Tất Thành - Đống Đa", QPointF(360, 26), "junction", "🌊");
    graph.addNode(87, "Nguyễn Tất Thành - Nguyễn Du", QPointF(530, 20), "junction", "🌊");
    graph.addNode(88, "Nguyễn Tất Thành - Như Nguyệt", QPointF(660, 20), "junction", "🌊");
    graph.addNode(89, "Trần Cao Vân - Cửa Ngõ Tây", QPointF(-20, 140), "junction", "📍");
    graph.addNode(90, "Trần Cao Vân - Hà Huy Tập", QPointF(60, 135), "junction", "📍");
    graph.addNode(91, "Trần Cao Vân - Thanh Khê Đông", QPointF(230, 128), "junction", "📍");
    graph.addNode(92, "Trần Cao Vân - Tam Thuận", QPointF(330, 125), "junction", "📍");

    // 6. CÁC NÚT BỔ SUNG KHU VỰC SÂN BAY & THANH KHÊ TÂY
    graph.addNode(93, "Nguyễn Tất Thành - Cửa Ngõ Cảng", QPointF(485, 21), "junction", "🌊");
    graph.addNode(94, "Xô Viết Nghệ Tĩnh - Điện Biên Phủ", QPointF(360, 270), "junction", "📍");
    graph.addNode(95, "Xô Viết Nghệ Tĩnh - Nguyễn Tất Thành", QPointF(360, 26), "junction", "🌊");
    graph.addNode(96, "Điện Biên Phủ - Xô Viết Nghệ Tĩnh Đông", QPointF(400, 270), "junction", "📍");
    graph.addNode(97, "Hà Huy Tập - Cửa Ngõ Nam", QPointF(60, 390), "junction", "📍");
    graph.addNode(98, "Nguyễn Hữu Dật - Điện Biên Phủ", QPointF(310, 270), "junction", "📍");
    graph.addNode(99, "Điện Biên Phủ - Ngã Tư Trường Chinh", QPointF(450, 270), "junction", "📍");
    graph.addNode(100, "Trần Cao Vân - Lê Độ", QPointF(400, 118), "junction", "📍");
    graph.addNode(101, "Lê Độ - Ông Ích Khiêm", QPointF(415, 150), "junction", "📍");
    graph.addNode(102, "Lê Độ - Hải Phòng Đông", QPointF(415, 175), "junction", "📍");
    graph.addNode(103, "Quang Trung - Bạch Đằng", QPointF(660, 80), "junction", "📍");
    graph.addNode(104, "Quang Trung - Trần Phú", QPointF(610, 80), "junction", "📍");
    graph.addNode(105, "Như Nguyệt - Phạm Văn Đồng Bắc", QPointF(775, 22), "junction", "📍");
    graph.addNode(106, "Phạm Văn Đồng - Trần Hưng Đạo Bắc", QPointF(830, 22), "junction", "📍");
    graph.addNode(107, "Phạm Văn Đồng - Đầu Bắc", QPointF(900, 22), "junction", "📍");
    graph.addNode(108, "An Hải Bắc - Đông Hưng Thuận", QPointF(890, 150), "junction", "📍");
    graph.addNode(109, "Trần Hưng Đạo - Phía Bắc Sơn Trà", QPointF(775, 45), "junction", "📍");
    graph.addNode(110, "Ngô Quyền Bắc - Mỹ Khê Bắc", QPointF(830, 60), "junction", "📍");
    graph.addNode(111, "Nguyễn Văn Thoại - Nam Kỳ Khởi Nghĩa", QPointF(1010, 640), "junction", "📍");
    graph.addNode(112, "Võ Nguyên Giáp - Võ Văn Kiệt", QPointF(1085, 380), "junction", "📍");

    auto dist = [](QPointF a, QPointF b) {
      return std::hypot(a.x() - b.x(), a.y() - b.y());
    };
    auto connect = [&](int u, int v, double traffic = 1.0, const QString &st = "",
                       bool bridge = false, bool major = true) {
      graph.addEdge(u, v, dist(graph.nodes[u].pos, graph.nodes[v].pos),
                    traffic, st, bridge, major);
    };

    // --- CÁC TUYẾN ĐƯỜNG BỜ TÂY & THANH KHÊ (LIÊN TỤC KHÔNG CỤT) ---
    // Tuyến đường bờ biển Nguyễn Tất Thành (uốn lượn suốt bờ vịnh)
    connect(0, 84, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(84, 85, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(85, 1, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(1, 86, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(86, 17, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(17, 87, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(87, 88, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(88, 46, 1.2, "Đ. Như Nguyệt - Bạch Đằng", false, true);

    // Tuyến Trần Cao Vân
    connect(89, 90, 1.2, "Đ. Trần Cao Vân", false, true);
    connect(90, 2, 1.2, "Đ. Trần Cao Vân", false, true);
    connect(2, 91, 1.3, "Đ. Trần Cao Vân", false, true);
    connect(91, 92, 1.3, "Đ. Trần Cao Vân", false, true);
    connect(92, 16, 1.3, "Đ. Trần Cao Vân", false, true);

    // Tuyến Hà Huy Tập
    connect(84, 90, 1.2, "Đ. Hà Huy Tập", false, true);
    connect(90, 81, 1.2, "Đ. Hà Huy Tập", false, true);

    // Tuyến Thái Thị Bôi
    connect(2, 82, 1.2, "Đ. Thái Thị Bôi", false, false);

    // Tuyến Điện Biên Phủ (kéo dài xuyên suốt Thanh Khê từ mạn Tây vào trung tâm)
    connect(3, 81, 1.2, "Đ. Điện Biên Phủ", false, true);
    connect(81, 82, 1.3, "Đ. Điện Biên Phủ", false, true);
    connect(82, 4, 1.3, "Đ. Điện Biên Phủ", false, true);
    connect(4, 83, 1.3, "Đ. Điện Biên Phủ", false, true);
    connect(83, 7, 1.4, "Đ. Điện Biên Phủ", false, true);
    connect(7, 14, 1.4, "Đ. Lý Thái Tổ", false, true);

    // Lê Duy Đình
    connect(83, 10, 1.1, "Đ. Lê Duy Đình", false, false);
    connect(10, 18, 1.1, "Đ. Lê Duy Đình", false, false);

    // Hải Phòng (qua Ga Đà Nẵng)
    connect(14, 15, 1.2, "Đ. Hải Phòng", false, true);
    connect(15, 33, 1.3, "Đ. Hải Phòng", false, true);
    connect(33, 40, 1.2, "Đ. Hải Phòng", false, true);

    // Ông Ích Khiêm
    connect(17, 16, 1.2, "Đ. Ông Ích Khiêm", false, true);
    connect(16, 15, 1.4, "Đ. Ông Ích Khiêm", false, true);
    connect(15, 25, 1.5, "Đ. Ông Ích Khiêm", false, true);
    connect(25, 24, 1.8, "Đ. Ông Ích Khiêm", false, true);

    // Lê Duẩn
    connect(14, 25, 1.4, "Đ. Lý Thái Tổ - Lê Duẩn", false, true);
    connect(25, 26, 1.6, "Đ. Lê Duẩn", false, true);
    connect(26, 34, 1.5, "Đ. Lê Duẩn", false, true);
    connect(34, 41, 1.4, "Đ. Lê Duẩn", false, true);
    connect(41, 47, 1.3, "Đ. Lê Duẩn", false, true);

    // Hùng Vương
    connect(14, 24, 1.5, "Đ. Hùng Vương", false, true);
    connect(24, 27, 1.9, "Đ. Hùng Vương", false, true);
    connect(27, 35, 1.7, "Đ. Hùng Vương", false, true);
    connect(35, 42, 1.8, "Đ. Hùng Vương", false, true);
    connect(42, 48, 1.5, "Đ. Hùng Vương", false, true);

    // Lê Lợi & Phan Châu Trinh
    connect(32, 33, 1.2, "Đ. Lê Lợi", false, true);
    connect(33, 34, 1.3, "Đ. Lê Lợi", false, true);
    connect(34, 35, 1.4, "Đ. Phan Châu Trinh", false, true);
    connect(35, 36, 1.4, "Đ. Phan Châu Trinh", false, true);
    connect(36, 37, 1.5, "Đ. Phan Châu Trinh", false, true);
    connect(37, 38, 1.3, "Đ. Phan Châu Trinh", false, true);

    // Hoàng Diệu & Hoàng Văn Thụ
    connect(26, 28, 1.5, "Đ. Hoàng Diệu", false, true);
    connect(28, 29, 1.6, "Đ. Hoàng Diệu", false, true);
    connect(29, 30, 1.4, "Đ. Hoàng Diệu", false, true);
    connect(28, 36, 1.3, "Đ. Hoàng Văn Thụ", false, false);
    connect(36, 44, 1.3, "Đ. Hoàng Văn Thụ", false, false);
    connect(44, 49, 1.4, "Đ. Hoàng Văn Thụ", false, false);

    // Trần Phú (trục 1 chiều Bắc - Nam)
    connect(39, 40, 1.3, "Đ. Trần Phú", false, true);
    connect(40, 41, 1.4, "Đ. Trần Phú", false, true);
    connect(41, 42, 1.6, "Đ. Trần Phú", false, true);
    connect(42, 43, 1.5, "Đ. Trần Phú", false, true);
    connect(43, 44, 1.4, "Đ. Trần Phú", false, true);
    connect(44, 45, 1.5, "Đ. Trần Phú", false, true);
    connect(45, 50, 1.3, "Đ. Trần Phú", false, true);

    // Bạch Đằng (trục ven sông bờ Tây tại X = 660, KHÔNG đè lên sông)
    connect(46, 47, 1.2, "Đ. Bạch Đằng", false, true);
    connect(47, 48, 1.3, "Đ. Bạch Đằng", false, true);
    connect(48, 49, 1.4, "Đ. Bạch Đằng", false, true);
    connect(49, 50, 1.2, "Đ. Bạch Đằng Nối Dài", false, true);
    connect(50, 51, 1.3, "Đ. 2 Tháng 9", false, true);
    connect(51, 52, 1.2, "Đ. 2 Tháng 9", false, true);
    connect(52, 53, 1.2, "Đ. 2 Tháng 9", false, true);

    // Đường ven Công viên 29/3 và Sân bay
    connect(7, 8, 1.1, "Đ. Nguyễn Tri Phương", false, true);
    connect(8, 10, 1.2, "Đ. Nguyễn Tri Phương", false, true);
    connect(10, 9, 1.2, "Đ. Nguyễn Tri Phương", false, true);
    connect(9, 11, 1.3, "Đ. Nguyễn Tri Phương", false, true);
    connect(18, 20, 1.2, "Đ. Phan Thanh", false, false);
    connect(18, 19, 1.1, "Đường ven Hồ Hàm Nghi", false, false);
    connect(19, 21, 1.2, "Đ. Hàm Nghi", false, false);

    // Đại lộ Nguyễn Văn Linh (trục lớn nối Cổng Sân Bay ↔ Cầu Rồng)
    connect(5, 11, 1.3, "Đ. Nguyễn Văn Linh", false, true);
    connect(11, 20, 1.5, "Đ. Nguyễn Văn Linh", false, true);
    connect(20, 21, 1.6, "Đ. Nguyễn Văn Linh", false, true);
    connect(21, 22, 1.6, "Đ. Nguyễn Văn Linh", false, true);
    connect(22, 31, 1.7, "Đ. Nguyễn Văn Linh", false, true);
    connect(31, 29, 1.8, "Đ. Nguyễn Văn Linh", false, true);
    connect(29, 37, 1.9, "Đ. Nguyễn Văn Linh", false, true);
    connect(37, 45, 1.8, "Đ. Nguyễn Văn Linh", false, true);
    connect(45, 49, 1.5, "Đ. Nguyễn Văn Linh", false, true);

    // Duy Tân
    connect(11, 12, 1.4, "Đ. Nguyễn Hữu Thọ", false, true);
    connect(13, 12, 1.2, "Đ. Duy Tân", false, true);
    connect(12, 23, 1.3, "Đ. Duy Tân", false, true);
    connect(23, 30, 1.3, "Đ. Duy Tân", false, true);
    connect(30, 53, 1.2, "Đ. Duy Tân", false, true);
    connect(53, 52, 1.2, "Đ. Duy Tân", false, true);
    connect(22, 23, 1.3, "Đ. Lê Đình Lý", false, false);

    // --- CÁC CÂY CẦU VƯỢT SÔNG HÀN (NỐI TỪ X=660 SANG X=775) ---
    connect(47, 54, 1.2, "Cầu Sông Hàn", true, true);
    connect(54, 57, 1.2, "Cầu Sông Hàn", true, true);
    connect(49, 55, 2.1, "Cầu Rồng", true, true);
    connect(55, 60, 2.0, "Cầu Rồng", true, true);
    connect(52, 56, 1.3, "Cầu Trần Thị Lý", true, true);
    connect(56, 62, 1.3, "Cầu Trần Thị Lý", true, true);

    // --- CÁC ĐƯỜNG TRỤC BỜ ĐÔNG (SƠN TRÀ & NGŨ HÀNH SƠN) ---
    // Trần Hưng Đạo (trục ven sông bờ Đông tại X = 775, KHÔNG đè lên sông)
    connect(59, 57, 1.2, "Đ. Trần Hưng Đạo", false, true);
    connect(57, 61, 1.3, "Đ. Trần Hưng Đạo", false, true);
    connect(61, 60, 1.4, "Đ. Trần Hưng Đạo", false, true);
    connect(60, 62, 1.3, "Đ. Trần Hưng Đạo", false, true);

    // Phạm Văn Đồng
    connect(57, 58, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(58, 63, 1.3, "Đ. Phạm Văn Đồng", false, true);
    connect(63, 70, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(70, 74, 1.1, "Đ. Phạm Văn Đồng", false, true);

    // Đại lộ Võ Văn Kiệt
    connect(60, 64, 1.4, "Đ. Võ Văn Kiệt", false, true);
    connect(64, 66, 1.5, "Đ. Võ Văn Kiệt", false, true);
    connect(66, 68, 1.4, "Đ. Võ Văn Kiệt", false, true);
    connect(68, 71, 1.3, "Đ. Võ Văn Kiệt", false, true);
    connect(71, 72, 1.3, "Đ. Võ Văn Kiệt", false, true);
    connect(72, 75, 1.2, "Đ. Võ Văn Kiệt", false, true);

    // Ngô Quyền (AH17)
    connect(63, 64, 1.4, "Đ. Ngô Quyền (AH17)", false, true);
    connect(64, 65, 1.4, "Đ. Ngô Quyền (AH17)", false, true);
    connect(65, 78, 1.3, "Đ. Ngũ Hành Sơn (AH17)", false, true);

    // Trần Thị Lý Đông & Đỗ Bá
    connect(62, 65, 1.3, "Đ. Trần Thị Lý Đông", false, true);
    connect(78, 79, 1.2, "Đ. Đỗ Bá", false, false);
    connect(79, 80, 1.2, "Đ. Đỗ Bá", false, false);
    connect(80, 77, 1.2, "Đ. Nguyễn Văn Thoại", false, true);

    // Đường nhánh bờ Đông
    connect(66, 67, 1.2, "Đ. Phạm Cự Lượng", false, false);
    connect(68, 69, 1.2, "Đ. Nguyễn Thiện Kế", false, false);
    connect(70, 71, 1.2, "Đ. Hồ Nghinh", false, false);
    connect(72, 73, 1.2, "Đ. Hà Bổng", false, false);
    connect(69, 73, 1.2, "Đ. Lê Hữu Trác", false, false);
    connect(73, 76, 1.2, "Đ. Lê Hữu Trác", false, false);

    // Đại lộ Võ Nguyên Giáp ven biển Mỹ Khê
    connect(74, 75, 1.2, "Đ. Võ Nguyên Giáp", false, true);
    connect(75, 76, 1.3, "Đ. Võ Nguyên Giáp", false, true);
    connect(76, 77, 1.3, "Đ. Võ Nguyên Giáp", false, true);

    // --- CÁC KẾT NỐI BỔ SUNG CHO NODES 93-112 ---
    // Nguyễn Tất Thành bổ sung (node 93 giữa 17 và 87)
    connect(17, 93, 1.1, "Đ. Nguyễn Tất Thành", false, true);
    connect(93, 87, 1.1, "Đ. Nguyễn Tất Thành", false, true);

    // Xô Viết Nghệ Tĩnh (nối Điện Biên Phủ ↔ Nguyễn Tất Thành tại X=360)
    // node 95 ≈ node 86 (Nguyễn Tất Thành - Đống Đa) - dùng chung
    connect(94, 96, 1.3, "Đ. Điện Biên Phủ", false, true);   // mở rộng DBP
    connect(83, 94, 1.3, "Đ. Điện Biên Phủ", false, true);   // Lê Duy Đình → Xô Viết NTĩnh
    connect(94, 14, 1.4, "Đ. Xô Viết Nghệ Tĩnh", false, true); // Xô Viết NTĩnh ↔ Lý Thái Tổ
    connect(86, 94, 1.2, "Đ. Xô Viết Nghệ Tĩnh", false, true); // Nguyễn Tất Thành → DBP qua XVNTĩnh
    connect(96, 7, 1.3, "Đ. Điện Biên Phủ", false, true);    // DBP → Nguyễn Tri Phương
    connect(98, 94, 1.3, "Đ. Điện Biên Phủ", false, true);   // Nguyễn Hữu Dật → XVNTĩnh
    connect(83, 98, 1.2, "Đ. Điện Biên Phủ", false, true);
    connect(99, 96, 1.4, "Đ. Điện Biên Phủ", false, true);   // Trường Chinh → Xô Viết
    connect(99, 25, 1.5, "Đ. Điện Biên Phủ", false, true);   // → Lê Duẩn nút trung tâm

    // Hà Huy Tập kéo dài xuống Nam (đến cổng sân bay)
    connect(81, 97, 1.3, "Đ. Hà Huy Tập", false, true);
    connect(97, 5, 1.2, "Đ. Hà Huy Tập", false, true);

    // Trần Cao Vân - Lê Độ nhánh
    connect(92, 100, 1.3, "Đ. Trần Cao Vân", false, true);
    connect(100, 16, 1.3, "Đ. Trần Cao Vân", false, true);   // nối vào Ông Ích Khiêm
    connect(100, 101, 1.2, "Đ. Lê Độ", false, false);
    connect(101, 15, 1.2, "Đ. Lê Độ", false, false);
    connect(101, 102, 1.2, "Đ. Lê Độ", false, false);
    connect(102, 15, 1.2, "Đ. Hải Phòng", false, true);

    // Quang Trung (nối Trần Phú ↔ Bạch Đằng theo chiều ngang)
    connect(104, 103, 1.3, "Đ. Quang Trung", false, true);
    connect(103, 46, 1.3, "Đ. Quang Trung", false, true);    // → Bạch Đằng
    connect(39, 104, 1.3, "Đ. Quang Trung", false, true);    // Trần Phú → Quang Trung
    connect(32, 104, 1.2, "Đ. Quang Trung", false, true);    // Lê Lợi → Quang Trung

    // Bờ Đông phía Bắc (Như Nguyệt → Phạm Văn Đồng)
    connect(88, 105, 1.2, "Đ. Như Nguyệt / Phạm Văn Đồng", false, true); // NTT→Phía Đông
    connect(105, 109, 1.2, "Đ. Trần Hưng Đạo", false, true);
    connect(109, 59, 1.2, "Đ. Trần Hưng Đạo", false, true);
    connect(105, 106, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(106, 110, 1.2, "Đ. Ngô Quyền", false, true);
    connect(110, 63, 1.2, "Đ. Ngô Quyền", false, true);
    connect(106, 107, 1.2, "Đ. Phạm Văn Đồng", false, true);
    connect(107, 108, 1.2, "Đ. Phạm Văn Đồng / An Hải", false, false);
    connect(108, 67, 1.2, "Đ. An Hải Bắc", false, false);
    connect(110, 58, 1.2, "Đ. Phạm Văn Đồng", false, true);

    // Nguyễn Văn Thoại (Ngũ Hành Sơn → Mỹ Khê)
    connect(80, 111, 1.2, "Đ. Nguyễn Văn Thoại", false, false);
    connect(111, 77, 1.2, "Đ. Nguyễn Văn Thoại", false, true);
    connect(77, 112, 1.2, "Đ. Võ Nguyên Giáp / Mỹ Khê", false, true);
    connect(112, 75, 1.2, "Đ. Võ Nguyên Giáp", false, true);
  }

  // ==================== VẼ BẢN ĐỒ CHI TIẾT (ĐỊA HÌNH, SÔNG, BIỂN, ĐƯỜNG XÃ) ====================
  void drawMap() {
    // 1. BIỂN ĐÔNG & BÃI BIỂN MỸ KHÊ (PHÍA ĐÔNG)
    QColor oceanColor = darkMode ? QColor(19, 32, 48) : QColor(168, 208, 224);
    auto *oceanRect = scene->addRect(1095, -80, 300, 880, QPen(Qt::NoPen), QBrush(oceanColor));
    oceanRect->setZValue(-32);

    // Bờ cát vàng óng ánh Bãi biển Mỹ Khê
    QColor sandColor = darkMode ? QColor(54, 48, 38) : QColor(252, 240, 206);
    auto *sandRect = scene->addRect(1075, -80, 20, 880, QPen(Qt::NoPen), QBrush(sandColor));
    sandRect->setZValue(-28);

    // Các đường gợn sóng biển Mỹ Khê
    QColor waveColor = darkMode ? QColor(28, 48, 72) : QColor(188, 222, 235);
    for (int wy = -40; wy < 780; wy += 80) {
      QPainterPath wave;
      wave.moveTo(1110, wy);
      wave.quadTo(1130, wy + 15, 1150, wy);
      wave.quadTo(1170, wy - 15, 1190, wy);
      auto *wItem = scene->addPath(wave, QPen(waveColor, 1.5, Qt::SolidLine, Qt::RoundCap));
      wItem->setZValue(-30);
    }

    auto *oceanText = scene->addText("B I Ể N   Đ Ô N G");
    QFont of("Segoe UI", 16, QFont::Bold);
    of.setLetterSpacing(QFont::AbsoluteSpacing, 6);
    oceanText->setFont(of);
    oceanText->setDefaultTextColor(darkMode ? QColor(55, 85, 120) : QColor(135, 180, 198));
    oceanText->setPos(1165, 230);
    oceanText->setRotation(90);
    oceanText->setZValue(-26);

    // 2. VỊNH ĐÀ NẴNG (MEN THEO CUNG ĐƯỜNG NGUYỄN TẤT THÀNH)
    QPainterPath bay;
    bay.moveTo(-40, -80);
    bay.lineTo(675, -80);
    bay.lineTo(675, 10);
    bay.cubicTo(560, 12, 445, 14, 360, 18);
    bay.cubicTo(280, 24, 185, 38, 100, 50);
    bay.cubicTo(50, 58, 0, 66, -40, 72);
    bay.closeSubpath();
    auto *bayItem = scene->addPath(bay, QPen(Qt::NoPen), QBrush(oceanColor));
    bayItem->setZValue(-32);

    auto *bayText = scene->addText("V Ị N H   Đ À   N Ẵ N G");
    QFont bf("Segoe UI", 12, QFont::Bold);
    bf.setLetterSpacing(QFont::AbsoluteSpacing, 5);
    bayText->setFont(bf);
    bayText->setDefaultTextColor(darkMode ? QColor(55, 85, 120) : QColor(135, 180, 198));
    bayText->setPos(100, -25);
    bayText->setZValue(-26);

    // 3. KHU VỰC ĐÔ THỊ BỜ ĐÔNG (QUẬN SƠN TRÀ & NGŨ HÀNH SƠN - ĐỒNG BẰNG ĐÔ THỊ)
    // Thực tế bản đồ trung tâm Đà Nẵng là khu phố bàn cờ bằng phẳng, tiếp giáp biển Mỹ Khê ở phía Đông

    // 4. SÔNG HÀN — Hai bờ uốn lượng theo thực tế Google Maps
    // Bờ Tây sông Hàn (x xung quanh 660–680, unh theo chiều dọc)
    QPainterPath riverLeft;
    riverLeft.moveTo(678, -80);
    riverLeft.cubicTo(673, -20, 668,  30, 672,  80);   // éo nhẹ vào trong phía Bắc
    riverLeft.cubicTo(676, 130, 670, 150, 668, 170);   // éo nhẹ ra ngoài
    riverLeft.cubicTo(664, 215, 660, 265, 658, 310);   // chạy xuống, lượn sang trái
    riverLeft.cubicTo(655, 355, 650, 395, 645, 430);   // tiếp tục unh trái
    riverLeft.cubicTo(640, 475, 644, 515, 648, 555);   // chạy xuống, lượn phải
    riverLeft.cubicTo(652, 590, 650, 620, 648, 660);   // xuống đáy
    riverLeft.lineTo(648, 760);

    // Bờ Đông sông Hàn (x xung quanh 760–780, uốn cượng ngược phía)
    QPainterPath riverRight;
    riverRight.moveTo(780, 760);
    riverRight.lineTo(780, 660);
    riverRight.cubicTo(778, 625, 776, 595, 774, 560);   // lượn nhẹ vào trong
    riverRight.cubicTo(772, 520, 774, 480, 776, 440);   // unh ra ngoài
    riverRight.cubicTo(778, 395, 775, 355, 770, 315);   // unh vào
    riverRight.cubicTo(765, 270, 762, 225, 763, 175);   // chạy lên
    riverRight.cubicTo(764, 145, 762, 115, 760,  80);   // unh nhẹ
    riverRight.cubicTo(758,  35, 756, -15, 760, -80);   // lên phía Bắc

    // Tạo vùng khép kín cho sông
    QPainterPath river;
    river.moveTo(678, -80);
    river.cubicTo(673, -20, 668,  30, 672,  80);
    river.cubicTo(676, 130, 670, 150, 668, 170);
    river.cubicTo(664, 215, 660, 265, 658, 310);
    river.cubicTo(655, 355, 650, 395, 645, 430);
    river.cubicTo(640, 475, 644, 515, 648, 555);
    river.cubicTo(652, 590, 650, 620, 648, 660);
    river.lineTo(648, 760);
    river.lineTo(780, 760);
    river.lineTo(780, 660);
    river.cubicTo(778, 625, 776, 595, 774, 560);
    river.cubicTo(772, 520, 774, 480, 776, 440);
    river.cubicTo(778, 395, 775, 355, 770, 315);
    river.cubicTo(765, 270, 762, 225, 763, 175);
    river.cubicTo(764, 145, 762, 115, 760,  80);
    river.cubicTo(758,  35, 756, -15, 760, -80);
    river.closeSubpath();

    QColor waterColor = darkMode ? QColor(22, 39, 64) : QColor(165, 208, 222);
    auto *riverItem = scene->addPath(river, QPen(Qt::NoPen), QBrush(waterColor));
    riverItem->setZValue(-25);

    auto *riverText = scene->addText("S   Ô   N   G      H   À   N");
    QFont rf("Segoe UI", 12, QFont::Bold);
    rf.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    riverText->setFont(rf);
    riverText->setDefaultTextColor(darkMode ? QColor(48, 80, 115) : QColor(125, 172, 190));
    riverText->setPos(710, 55);
    riverText->setRotation(78);
    riverText->setZValue(-24);

    // Đảo Xanh / Euro Village ven sông phía Nam
    QColor islandColor = darkMode ? QColor(36, 48, 38) : QColor(210, 235, 210);
    QPainterPath islandPath;
    islandPath.addRoundedRect(QRectF(690, 645, 48, 75), 14, 14);
    auto *island = scene->addPath(islandPath, QPen(Qt::NoPen), QBrush(islandColor));
    island->setZValue(-23);
    auto *islandText = scene->addText("Đảo Xanh");
    QFont isf("Segoe UI", 7, QFont::Bold);
    islandText->setFont(isf);
    islandText->setDefaultTextColor(darkMode ? QColor(120, 170, 120) : QColor(40, 95, 40));
    islandText->setPos(692, 676);
    islandText->setZValue(-22);

    // 5. CÔNG VIÊN 29/3 VÀ HỒ NƯỚC CẢNH QUAN
    QColor parkColor = darkMode ? QColor(26, 52, 34) : QColor(204, 235, 205);
    QPainterPath parkPath;
    parkPath.addRoundedRect(QRectF(230, 280, 80, 130), 16, 16);
    auto *park293 = scene->addPath(parkPath, QPen(Qt::NoPen), QBrush(parkColor));
    park293->setZValue(-22);

    QPainterPath lake293;
    lake293.moveTo(270, 295);
    lake293.cubicTo(245, 325, 245, 365, 265, 395);
    lake293.cubicTo(285, 400, 295, 375, 285, 345);
    lake293.cubicTo(278, 325, 290, 305, 270, 295);
    auto *lake293Item = scene->addPath(lake293, QPen(Qt::NoPen), QBrush(waterColor));
    lake293Item->setZValue(-21);

    auto *t293 = scene->addText("Hồ Công viên 29/3");
    QFont pf("Segoe UI", 7, QFont::DemiBold);
    t293->setFont(pf);
    t293->setDefaultTextColor(darkMode ? QColor(70, 110, 140) : QColor(80, 135, 150));
    t293->setPos(235, 335);
    t293->setZValue(-20);

    // Hồ Thạc Gián & Hồ Hàm Nghi
    QPainterPath tgPath;
    tgPath.addRoundedRect(QRectF(348, 310, 28, 48), 8, 8);
    auto *lakeThacGian = scene->addPath(tgPath, QPen(Qt::NoPen), QBrush(waterColor));
    lakeThacGian->setZValue(-21);

    QPainterPath hgPath;
    hgPath.addRoundedRect(QRectF(395, 310, 28, 48), 8, 8);
    auto *lakeHamNghi = scene->addPath(hgPath, QPen(Qt::NoPen), QBrush(waterColor));
    lakeHamNghi->setZValue(-21);

    auto *tHN = scene->addText("Hồ Hàm Nghi");
    tHN->setFont(pf);
    tHN->setDefaultTextColor(darkMode ? QColor(70, 110, 140) : QColor(80, 135, 150));
    tHN->setPos(370, 358);
    tHN->setZValue(-20);

    // Công viên APEC với Mái vòm "Cánh Diều Bay Cao"
    auto *apecPark = scene->addEllipse(615, 395, 40, 35, QPen(Qt::NoPen), QBrush(parkColor));
    apecPark->setZValue(-22);

    // Mái vòm uốn lượn trắng APEC
    QPainterPath kiteDome;
    kiteDome.moveTo(622, 412);
    kiteDome.cubicTo(630, 398, 642, 398, 650, 412);
    kiteDome.cubicTo(642, 420, 630, 420, 622, 412);
    auto *domeItem = scene->addPath(kiteDome, QPen(QColor(180, 180, 180), 1), QBrush(Qt::white));
    domeItem->setZValue(-21);

    // 6. CẢNG HÀNG KHÔNG QUỐC TẾ ĐÀ NẴNG (SÂN BAY CHI TIẾT)
    QColor airportBg = darkMode ? QColor(36, 38, 48) : QColor(228, 224, 216);
    auto *airportArea = scene->addRect(20, 390, 190, 290,
                                       QPen(QColor(180, 175, 165), 1),
                                       QBrush(airportBg));
    airportArea->setZValue(-20);

    // Sân đỗ máy bay (Apron)
    QColor apronColor = darkMode ? QColor(48, 50, 58) : QColor(210, 206, 198);
    auto *apron = scene->addRect(125, 435, 75, 195, QPen(Qt::NoPen), QBrush(apronColor));
    apron->setZValue(-19);

    // Đường băng 35R/17L
    auto *runway1 = scene->addRect(85, 410, 20, 250,
                                   QPen(Qt::NoPen),
                                   QBrush(darkMode ? QColor(60, 62, 70) : QColor(85, 90, 98)));
    runway1->setZValue(-19);
    auto *rLine1 = scene->addLine(95, 418, 95, 652, QPen(Qt::white, 1.5, Qt::DashLine));
    rLine1->setZValue(-18);

    // Số hiệu đường băng
    auto *rNum1 = scene->addText("35R");
    rNum1->setFont(QFont("Segoe UI", 6, QFont::Bold));
    rNum1->setDefaultTextColor(Qt::white);
    rNum1->setPos(87, 412);
    rNum1->setZValue(-17);

    // Đường băng 35L/17R
    auto *runway2 = scene->addRect(50, 430, 18, 210,
                                   QPen(Qt::NoPen),
                                   QBrush(darkMode ? QColor(55, 57, 65) : QColor(95, 100, 108)));
    runway2->setZValue(-19);
    auto *rLine2 = scene->addLine(59, 436, 59, 634, QPen(Qt::white, 1.5, Qt::DashLine));
    rLine2->setZValue(-18);

    // Các đường lăn taxiways
    QPen taxiPen(darkMode ? QColor(70, 72, 80) : QColor(130, 135, 142), 5, Qt::SolidLine, Qt::RoundCap);
    auto *taxi1 = scene->addLine(95, 475, 135, 475, taxiPen);
    taxi1->setZValue(-18);
    auto *taxi2 = scene->addLine(95, 545, 135, 545, taxiPen);
    taxi2->setZValue(-18);

    // Máy bay đỗ ở sân đỗ (Apron)
    auto *plane1 = scene->addText("✈️");
    plane1->setFont(QFont("Segoe UI", 11));
    plane1->setPos(140, 450);
    plane1->setZValue(-17);

    auto *plane2 = scene->addText("✈️");
    plane2->setFont(QFont("Segoe UI", 11));
    plane2->setPos(140, 520);
    plane2->setZValue(-17);

    // Nhà ga hành khách T1 (Quốc nội) & T2 (Quốc tế)
    QPainterPath termPath;
    termPath.addRoundedRect(QRectF(145, 580, 55, 50), 6, 6);
    auto *terminal = scene->addPath(termPath,
                                    QPen(QColor(150, 145, 135), 1),
                                    QBrush(darkMode ? QColor(50, 52, 60) : QColor(245, 245, 250)));
    terminal->setZValue(-18);

    auto *apText = scene->addText("Cảng hàng không QT Đà Nẵng\n(Ga T1 & T2)");
    QFont apf("Segoe UI", 8, QFont::Bold);
    apText->setFont(apf);
    apText->setDefaultTextColor(darkMode ? QColor(160, 165, 180) : QColor(100, 105, 115));
    apText->setPos(40, 665);
    apText->setZValue(-17);

    // 7. HỆ THỐNG MẶT ĐƯỜNG PHÂN CẤP CHUẨN GOOGLE MAPS (THẲNG VÀ BÁM SÁT MẠNG LƯỚI ĐÔ THỊ)
    // Lớp 1: Casing (Viền ngoài mặt đường)
    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (const Edge &e : graph.adj[u]) {
        if (u < e.to) {
          int outW = e.isMajor ? 16 : 10;
          auto *line = scene->addLine(QLineF(graph.nodes[u].pos, graph.nodes[e.to].pos),
              QPen(roadColor, outW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
          line->setZValue(-10);
        }
      }
    }

    // Lớp 2: Fill (Đại lộ màu vàng Google Maps, đường nội bộ màu trắng)
    QColor majorFill = darkMode ? QColor(80, 72, 56) : QColor(254, 225, 153);
    QColor minorFill = darkMode ? QColor(48, 47, 44) : QColor(255, 255, 255);

    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (const Edge &e : graph.adj[u]) {
        if (u < e.to) {
          int inW = e.isMajor ? 11 : 6;
          QColor fillC = e.isMajor ? majorFill : minorFill;
          auto *line = scene->addLine(QLineF(graph.nodes[u].pos, graph.nodes[e.to].pos),
              QPen(fillC, inW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
          line->setZValue(-9);
        }
      }
    }

    // 8. CÁC CÂY CẦU NGHỆ THUẬT DANH TIẾNG ĐÀ NẴNG
    // 8.A. CẦU THUẬN PHƯỚC (CẦU TREO DÂY VÕNG LỚN NHẤT VIỆT NAM Ở CỬA SÔNG HÀN)
    // 2 Trụ tháp cầu treo màu bạc
    QPen towerPen(darkMode ? QColor(180, 185, 200) : QColor(140, 150, 165), 3.5, Qt::SolidLine, Qt::SquareCap);
    auto *tower1 = scene->addLine(685, -16, 685, 18, towerPen);
    tower1->setZValue(-3);
    auto *tower2 = scene->addLine(748, -16, 748, 18, towerPen);
    tower2->setZValue(-3);

    // 2 Cáp võng chính parabol uốn lượn qua đỉnh tháp
    QPainterPath cable1;
    cable1.moveTo(660, 8);
    cable1.quadTo(685, -16, 716, 2);
    cable1.quadTo(748, -16, 770, 8);
    auto *cItem1 = scene->addPath(cable1, QPen(QColor(100, 181, 246), 2.2, Qt::SolidLine, Qt::RoundCap));
    cItem1->setZValue(-2);

    // Dây treo thẳng đứng
    for (int sx = 692; sx <= 742; sx += 8) {
      double t = (sx - 685.0) / (748.0 - 685.0);
      double cy = -16.0 + 4.0 * 18.0 * (t - 0.5) * (t - 0.5);
      auto *hanger = scene->addLine(sx, cy, sx, 8, QPen(QColor(140, 150, 165, 160), 1));
      hanger->setZValue(-3);
    }

    // 8.B. CẦU SÔNG HÀN (CẦU QUAY ĐẦU TIÊN TẠI VIỆT NAM)
    // Trụ xoay tròn ở chính giữa lòng sông
    auto *pivotIsland = scene->addEllipse(711, 138, 14, 14,
                                          QPen(darkMode ? QColor(120, 130, 145) : QColor(180, 185, 195), 1.5),
                                          QBrush(darkMode ? QColor(70, 75, 88) : QColor(220, 225, 232)));
    pivotIsland->setZValue(-3);

    // Giàn thép chịu lực vòm 2 bên trụ xoay
    QPainterPath swingArch1, swingArch2;
    swingArch1.moveTo(675, 145);
    swingArch1.quadTo(696, 130, 718, 145);
    swingArch2.moveTo(718, 145);
    swingArch2.quadTo(740, 130, 762, 145);
    auto *archItem1 = scene->addPath(swingArch1, QPen(QColor(66, 133, 244), 3, Qt::SolidLine, Qt::RoundCap));
    auto *archItem2 = scene->addPath(swingArch2, QPen(QColor(66, 133, 244), 3, Qt::SolidLine, Qt::RoundCap));
    archItem1->setZValue(-2);
    archItem2->setZValue(-2);

    // 8.C. CẦU RỒNG (BIỂU TƯỢNG VÀNG RỰC RỠ CỦA ĐÀ NẴNG)
    // Thân rồng vàng 3 nhịp vòm thép uốn sóng uyển chuyển
    QPainterPath dragonBody;
    dragonBody.moveTo(655, 375);
    dragonBody.quadTo(685, 342, 715, 375);
    dragonBody.quadTo(745, 345, 775, 375);
    auto *dragon = scene->addPath(dragonBody, QPen(QColor(255, 179, 0), 5.5, Qt::SolidLine, Qt::RoundCap));
    dragon->setZValue(-2);

    // Đầu rồng vươn cao ở mố phía Đông hướng ra Biển Đông
    QPainterPath dragonHead;
    dragonHead.moveTo(775, 375);
    dragonHead.lineTo(788, 366);
    dragonHead.lineTo(796, 368);
    dragonHead.lineTo(790, 376);
    dragonHead.closeSubpath();
    auto *headItem = scene->addPath(dragonHead, QPen(Qt::NoPen), QBrush(QColor(255, 179, 0)));
    headItem->setZValue(-1);

    // Vệt hào quang / lửa đầu rồng
    auto *dragonGlow = scene->addEllipse(790, 365, 8, 8, QPen(Qt::NoPen), QBrush(QColor(255, 87, 34, 180)));
    dragonGlow->setZValue(-1);

    // Đuôi rồng hình hoa sen cách điệu ở mố phía Tây
    QPainterPath dragonTail;
    dragonTail.moveTo(655, 375);
    dragonTail.lineTo(646, 368);
    dragonTail.lineTo(644, 375);
    dragonTail.lineTo(646, 382);
    dragonTail.closeSubpath();
    auto *tailItem = scene->addPath(dragonTail, QPen(Qt::NoPen), QBrush(QColor(255, 179, 0)));
    tailItem->setZValue(-1);

    // 8.D. CẦU TRẦN THỊ LÝ (TRỤ THÁP NGHIÊNG CÁNH BUỒM DÂY VĂNG)
    // Trụ tháp nghiêng chữ Y ngược cao vút nghiêng 12 độ
    QPen pylonPen(QColor(244, 81, 30), 4.5, Qt::SolidLine, Qt::RoundCap);
    auto *pylon = scene->addLine(710, 610, 712, 565, pylonPen);
    pylon->setZValue(-2);

    // Chùm dây văng nan quạt xòe như cánh buồm hướng ra biển
    QPen fanPen(QColor(255, 112, 67, 180), 1.2);
    for (int bx = 665; bx <= 755; bx += 10) {
      if (std::abs(bx - 710) > 6) {
        auto *fanLine = scene->addLine(712, 567, bx, 610, fanPen);
        fanLine->setZValue(-3);
      }
    }

    // 9. NÚT GIAO LỘ (JUNCTION DOTS)
    for (const Node &n : graph.nodes) {
      QColor dotC = darkMode ? QColor(105, 100, 92) : QColor(185, 178, 165);
      auto *dot = scene->addEllipse(n.pos.x() - 2.5, n.pos.y() - 2.5, 5, 5,
                                    QPen(Qt::NoPen), QBrush(dotC));
      dot->setZValue(4);
    }

    // 10. ĐỊA DANH & POI NỔI BẬT
    poiGroup = scene->createItemGroup({});

    struct LandmarkItem {
      QString name;
      QString icon;
      QPointF pos;
      QColor badgeBg;
      QPointF offset;
    };
    QVector<LandmarkItem> pois = {
      {"Cảng hàng không quốc tế Đà Nẵng", "✈️", QPointF(90, 520), QColor(26, 115, 232), QPointF(-40, 14)},
      {"Ga Đà Nẵng", "🚆", QPointF(370, 180), QColor(0, 137, 123), QPointF(10, -7)},
      {"Hồ Công viên 29/3", "🌳", QPointF(265, 340), QColor(46, 125, 50), QPointF(10, -7)},
      {"Hồ Hàm Nghi", "💧", QPointF(385, 330), QColor(3, 155, 229), QPointF(8, -7)},
      {"AEON MALL Đà Nẵng Thanh Khê", "🛍️", QPointF(175, 240), QColor(230, 81, 0), QPointF(10, -10)},
      {"Chợ Cồn", "🛍️", QPointF(450, 275), QColor(230, 81, 0), QPointF(10, -7)},
      {"Chợ Hàn", "🛍️", QPointF(610, 275), QColor(230, 81, 0), QPointF(10, -7)},
      {"Giáo Xứ Chính Toà Đà Nẵng", "⛪", QPointF(610, 320), QColor(94, 53, 177), QPointF(10, -8)},
      {"Vincom Plaza Đà Nẵng", "🛍️", QPointF(800, 150), QColor(230, 81, 0), QPointF(10, -7)},
      {"Công Viên APEC", "🌳", QPointF(625, 410), QColor(46, 125, 50), QPointF(10, 8)},
      {"Bãi biển Mỹ Khê", "🏖️", QPointF(1085, 380), QColor(0, 151, 167), QPointF(-100, -7)},
      {"Cầu Thuận Phước", "🌉", QPointF(716, 8), QColor(30, 136, 229), QPointF(-36, -20)},
      {"Cầu Sông Hàn", "🌉", QPointF(718, 145), QColor(245, 124, 0), QPointF(-32, -18)},
      {"Cầu Rồng", "🐉", QPointF(715, 375), QColor(245, 124, 0), QPointF(-26, -18)},
      {"Cầu Trần Thị Lý", "🌉", QPointF(710, 610), QColor(244, 81, 30), QPointF(-36, -18)},
    };
    for (const auto &pi : pois) {
      auto *badge = scene->addEllipse(pi.pos.x() - 6.5, pi.pos.y() - 6.5, 13, 13,
                                      QPen(Qt::white, 1.5), QBrush(pi.badgeBg));
      badge->setZValue(12);
      poiGroup->addToGroup(badge);

      auto *t = scene->addText(pi.icon + " " + pi.name);
      QFont pf("Segoe UI", 8, QFont::DemiBold);
      t->setFont(pf);
      t->setPos(pi.pos + pi.offset);
      t->setDefaultTextColor(darkMode ? QColor(230, 235, 245) : QColor(32, 33, 36));
      t->setZValue(13);
      poiGroup->addToGroup(t);
    }
    poiGroup->setVisible(showLandmarksLayer);

    // 11. TÊN ĐƯỜNG PHỐ (STREET LABELS)
    streetLabelGroup = scene->createItemGroup({});

    struct StreetLabel {
      QString name;
      QPointF pos;
      double rotation;
      bool isMajor;
    };
    QVector<StreetLabel> streetLabels = {
      // Bờ Tây & Trung tâm
      {"Nguyễn Tất Thành", QPointF(130, 40), -8.0, true},
      {"Nguyễn Tất Thành", QPointF(290, 20), -6.0, true},
      {"Trần Cao Vân", QPointF(65, 126), 0.0, false},
      {"Trần Cao Vân", QPointF(320, 118), 0.0, false},
      {"Hải Phòng", QPointF(420, 166), 0.0, false},
      {"Điện Biên Phủ", QPointF(70, 252), 0.0, true},
      {"Lê Duy Đình", QPointF(175, 338), 0.0, false},
      {"Nguyễn Tri Phương", QPointF(280, 375), 90.0, true},
      {"Phan Thanh", QPointF(350, 400), 90.0, false},
      {"Hàm Nghi", QPointF(400, 400), 90.0, false},
      {"Lê Đình Lý", QPointF(440, 520), 90.0, false},
      {"Nguyễn Hữu Thọ", QPointF(330, 570), 90.0, true},
      {"Duy Tân", QPointF(270, 602), 0.0, true},
      {"Duy Tân", QPointF(415, 602), 0.0, true},
      {"Ông Ích Khiêm", QPointF(440, 95), 90.0, false},
      {"Hùng Vương", QPointF(335, 267), 0.0, true},
      {"Ng. Gia Tự", QPointF(480, 245), 90.0, false},
      {"Triệu Nữ Vương", QPointF(460, 390), 90.0, false},
      {"Hoàng Diệu", QPointF(480, 490), 90.0, true},
      {"Hoàng Văn Thụ", QPointF(520, 357), 0.0, false},
      {"Lê Lợi", QPointF(520, 130), 90.0, false},
      {"Phan Châu Trinh", QPointF(520, 310), 90.0, false},
      {"Phan Châu Trinh", QPointF(522, 490), 90.0, false},
      {"Lê Duẩn", QPointF(380, 212), 0.0, true},
      {"Trần Phú", QPointF(600, 210), 90.0, true},
      {"Trần Phú", QPointF(600, 335), 90.0, true},
      {"Bạch Đằng", QPointF(655, 210), 90.0, true},
      {"Bạch Đằng", QPointF(655, 335), 90.0, true},
      {"Nguyễn Văn Linh", QPointF(380, 432), 0.0, true},
      {"Đ. 2 Tháng 9", QPointF(615, 530), 90.0, true},
      {"Trần Thị Lý", QPointF(560, 602), 0.0, true},

      // Bờ Đông
      {"Đ. Trần Hưng Đạo", QPointF(755, 260), 90.0, true},
      {"Đ. Trần Hưng Đạo", QPointF(758, 480), 90.0, true},
      {"Võ Văn Kiệt", QPointF(815, 372), 0.0, true},
      {"AH17", QPointF(820, 260), 90.0, true},
      {"Phạm Cự Lượng", QPointF(880, 330), 90.0, false},
      {"Nguyễn Thiện Kế", QPointF(930, 330), 90.0, false},
      {"Lê Hữu Trác", QPointF(870, 452), 0.0, false},
      {"Chính Hữu", QPointF(965, 210), 90.0, false},
      {"Morrison", QPointF(945, 175), 0.0, false},
      {"Hồ Nghinh", QPointF(1000, 260), 90.0, false},
      {"Võ Nguyên Giáp", QPointF(1075, 260), 90.0, true},
      {"Trần Thị Lý", QPointF(780, 602), 0.0, true},
      {"Đỗ Bá", QPointF(890, 632), 0.0, false},
      {"Dương Khuê", QPointF(765, 632), 0.0, false},

      // Đường bổ sung bờ Đông
      {"Phạm Văn Đồng", QPointF(830, -18), 0.0, true},
      {"Trần Hưng Đạo", QPointF(757, 35), 90.0, true},
      {"Ngô Quyền", QPointF(820, 35), 90.0, true},
      {"An Hải Bắc", QPointF(880, 100), 90.0, false},

      // Tuyến đường khác
      {"Quang Trung", QPointF(560, 72), 0.0, false},
      {"Xô Viết Nghệ Tĩnh", QPointF(350, 235), 90.0, true},
      {"Nguyễn Hữu Dật", QPointF(295, 252), 90.0, false},
      {"Lê Độ", QPointF(410, 135), 90.0, false},
      {"Nguyễn Tất Thành", QPointF(435, 8), -5.0, true},
      {"Như Nguyệt", QPointF(645, 5), 0.0, false},
      {"Hà Huy Tập", QPointF(50, 325), 90.0, false},
      {"Nguyễn Văn Thoại", QPointF(1000, 632), 0.0, false}
    };
    for (const auto &sl : streetLabels) {
      auto *st = scene->addText(sl.name);
      QFont sf("Segoe UI", sl.isMajor ? 8 : 7, sl.isMajor ? QFont::Bold : QFont::DemiBold);
      st->setFont(sf);
      st->setDefaultTextColor(sl.isMajor
                                  ? (darkMode ? QColor(230, 235, 245) : QColor(32, 33, 36))
                                  : (darkMode ? QColor(165, 170, 180) : QColor(95, 99, 104)));
      st->setPos(sl.pos);
      st->setRotation(sl.rotation);
      st->setZValue(7);
      streetLabelGroup->addToGroup(st);
    }
    streetLabelGroup->setVisible(showStreetLabelsLayer);

    // 12. TÊN PHƯỜNG & KHU VỰC
    struct DistrictLabel {
      QString name;
      QPointF pos;
    };
    QVector<DistrictLabel> distLabels = {
      {"THANH KHÊ", QPointF(35, 235)},
      {"PHÚ GIA", QPointF(350, 90)},
      {"AN HẢI", QPointF(710, 305)},
      {"HÒA CƯƠNG", QPointF(475, 665)},
      {"BẮC MỸ PHÚ", QPointF(855, 665)},
      {"KHU DU LỊCH VEN BIỂN", QPointF(995, 305)}
    };
    for (const auto &dl : distLabels) {
      auto *dt = scene->addText(dl.name);
      QFont df("Segoe UI", 8, QFont::DemiBold);
      df.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
      dt->setFont(df);
      dt->setDefaultTextColor(darkMode ? QColor(135, 140, 155) : QColor(120, 125, 135));
      dt->setPos(dl.pos);
      dt->setZValue(2);
    }

    // 13. GRID TỌA ĐỘ
    gridGroup = scene->createItemGroup({});
    QColor gridC = darkMode ? QColor(48, 48, 58) : QColor(215, 209, 198);
    QPen gridPen(gridC, 0.5);
    for (int x = -40; x < 1280; x += 60)
      gridGroup->addToGroup(scene->addLine(x, -80, x, 780, gridPen));
    for (int y = -80; y < 780; y += 60)
      gridGroup->addToGroup(scene->addLine(-40, y, 1280, y, gridPen));
    gridGroup->setZValue(-35);
    gridGroup->setVisible(gridVisible);

    // 14. BẢNG CHÚ THÍCH MẬT ĐỘ (LEGEND)
    legendGroup = scene->createItemGroup({});
    QColor legendBg  = darkMode ? QColor(32, 34, 48, 230) : QColor(255, 255, 255, 230);
    QColor legendBdr = darkMode ? QColor(68, 70, 90)       : QColor(218, 220, 224);
    auto *lRect = scene->addRect(14, 68, 215, 78, QPen(legendBdr, 1), QBrush(legendBg));
    lRect->setZValue(49);
    legendGroup->addToGroup(lRect);

    struct LI { QString label; QColor color; };
    QVector<LI> legendItems = {
      {"● Xanh: Thông thoáng (≤1.2x)",  QColor(15, 157, 88)},
      {"● Vàng/Cam: Đông vừa (≤1.9x)",  QColor(251, 188, 4)},
      {"● Đỏ: Ùn tắc (≥2.0x)",         QColor(234, 67, 53)},
    };
    int ly = 73;
    for (const auto &li : legendItems) {
      auto *t = scene->addText(li.label);
      QFont lf("Segoe UI", 8, QFont::DemiBold);
      t->setFont(lf);
      t->setPos(18, ly);
      t->setDefaultTextColor(li.color);
      t->setZValue(50);
      legendGroup->addToGroup(t);
      ly += 21;
    }
    legendGroup->setVisible(showTrafficLayer);

    scene->setSceneRect(-40, -80, 1320, 860);
  }

  void applyTheme() {
    if (darkMode) {
      setBackgroundBrush(QColor(24, 26, 32));
      roadColor = QColor(64, 60, 54);
      textColor = QColor(218, 220, 225);
    } else {
      setBackgroundBrush(QColor(242, 239, 233));
      roadColor = QColor(196, 190, 176);
      textColor = QColor(60, 64, 67);
    }

    updateFloatingControlTheme();
    updateFloatingSearchBarTheme();
    updateFloatingRouteBannerTheme();

    stopAnimation();
    stopPathAnim();

    pathItems.clear();
    markerItems.clear();
    altPathItems.clear();
    secondaryPathItems.clear();
    pathAnimLines.clear();
    gridGroup = nullptr;
    poiGroup = nullptr;
    streetLabelGroup = nullptr;
    legendGroup = nullptr;

    scene->clear();
    drawMap();

    if (!startPoint.isNull() || !endPoint.isNull())
      redrawMarkers();
    if (!lastSteps.isEmpty() || !timeSteps.isEmpty() || !distSteps.isEmpty())
      recompute();

    viewport()->update();
    emit statusMessage(darkMode ? "Chế độ ban đêm (Dark Mode) bật" : "Chế độ ban ngày (Light Mode) bật");
  }

  void addMarker(QPointF pos, const QColor &color, const QString &label) {
    const double r = 11.0;
    QPainterPath pin;
    pin.addEllipse(QRectF(-r, -r * 2.6, r * 2.0, r * 2.0));
    pin.moveTo(-r * 0.5, -r * 0.65);
    pin.lineTo(r * 0.5, -r * 0.65);
    pin.lineTo(0.0, 0.0);
    pin.closeSubpath();

    auto *pinItem = scene->addPath(pin, QPen(color.darker(160), 1.5), QBrush(color));
    pinItem->setPos(pos);
    pinItem->setZValue(30);
    markerItems.append(pinItem);

    const double dotR = r * 0.38;
    auto *inner = scene->addEllipse(
        pos.x() - dotR, pos.y() - r * 1.6 - dotR,
        dotR * 2.0, dotR * 2.0,
        QPen(Qt::NoPen), QBrush(Qt::white));
    inner->setZValue(31);
    markerItems.append(inner);

    auto *t = scene->addSimpleText(label);
    QFont lf("Segoe UI", 10, QFont::Bold);
    t->setFont(lf);
    t->setBrush(color.darker(140));
    t->setPos(pos.x() - 4, pos.y() - r * 2.6 - 14);
    t->setZValue(32);
    markerItems.append(t);
  }

  void clearMarkers() {
    for (auto *i : markerItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    markerItems.clear();
  }

  void redrawMarkers() {
    if (!startPoint.isNull())
      addMarker(startPoint, colorA, "A");
    if (!endPoint.isNull())
      addMarker(endPoint, colorB, "B");
  }

  void clearPath() {
    stopPathAnim();
    stopAnimation();

    for (auto *i : pathItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    pathItems.clear();

    for (auto *i : altPathItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    altPathItems.clear();

    for (auto *i : secondaryPathItems) {
      if (i && i->scene())
        scene->removeItem(i);
      delete i;
    }
    secondaryPathItems.clear();
    pathAnimLines.clear();
  }

  int findNearestNode(QPointF p, double maxDist) const {
    int best = -1;
    double bestD = maxDist;
    for (const Node &n : graph.nodes) {
      double d = QLineF(p, n.pos).length();
      if (d < bestD) {
        bestD = d;
        best = n.id;
      }
    }
    return best;
  }

  std::tuple<int, int, QPointF> findNearestPointOnEdge(QPointF click) {
    double bestDist = 26.0;
    int bestU = -1, bestV = -1;
    QPointF bestProj;

    for (int u = 0; u < graph.nodes.size(); ++u) {
      for (const Edge &e : graph.adj[u]) {
        if (u >= e.to)
          continue;
        QPointF A = graph.nodes[u].pos, B = graph.nodes[e.to].pos;
        QPointF AP = click - A, AB = B - A;
        double ab2 = QPointF::dotProduct(AB, AB);
        if (ab2 < 1e-6)
          continue;
        double t = std::clamp(QPointF::dotProduct(AP, AB) / ab2, 0.0, 1.0);
        QPointF proj = A + t * AB;
        double d = QLineF(click, proj).length();
        if (d < bestDist) {
          bestDist = d;
          bestU = u;
          bestV = e.to;
          bestProj = proj;
        }
      }
    }
    return {bestU, bestV, bestProj};
  }

  double trafficOf(const Graph &g, int u, int v) const {
    if (u < 0 || v < 0)
      return 1.0;
    for (const Edge &e : g.adj[u])
      if (e.to == v)
        return e.trafficFactor;
    return 1.0;
  }

  QString streetOf(const Graph &g, int u, int v) const {
    if (u < 0 || v < 0)
      return "";
    for (const Edge &e : g.adj[u])
      if (e.to == v)
        return e.streetName;
    return "";
  }

  bool isBridgeOf(const Graph &g, int u, int v) const {
    if (u < 0 || v < 0)
      return false;
    for (const Edge &e : g.adj[u])
      if (e.to == v)
        return e.isBridge;
    return false;
  }

  void setEndpointFromNode(int nid) {
    if (startPoint.isNull()) {
      startPoint = graph.nodes[nid].pos;
      startEdgeU = nid;
      startEdgeV = -1;
      clearPath();
      clearMarkers();
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(nid, -1);
    } else if (endPoint.isNull()) {
      endPoint = graph.nodes[nid].pos;
      endEdgeU = nid;
      endEdgeV = -1;
      addMarker(endPoint, colorB, "B");
      recompute();
      emit endpointsChanged(startEdgeU, nid);
    } else {
      clearPath();
      clearMarkers();
      stopAnimation();
      stopPathAnim();
      startPoint = graph.nodes[nid].pos;
      startEdgeU = nid;
      startEdgeV = -1;
      endPoint = QPointF();
      endEdgeU = endEdgeV = -1;
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(nid, -1);
    }
    emit routeChanged();
  }

  void setEndpointFromEdge(int edgeU, int edgeV, QPointF projected) {
    if (startPoint.isNull()) {
      startPoint = projected;
      startEdgeU = edgeU;
      startEdgeV = edgeV;
      clearPath();
      clearMarkers();
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(edgeU, -1);
    } else if (endPoint.isNull()) {
      endPoint = projected;
      endEdgeU = edgeU;
      endEdgeV = edgeV;
      addMarker(endPoint, colorB, "B");
      recompute();
      emit endpointsChanged(startEdgeU, edgeU);
    } else {
      clearPath();
      clearMarkers();
      stopAnimation();
      stopPathAnim();
      startPoint = projected;
      startEdgeU = edgeU;
      startEdgeV = edgeV;
      endPoint = QPointF();
      endEdgeU = endEdgeV = -1;
      addMarker(startPoint, colorA, "A");
      lastSteps.clear();
      totalMeters = totalSeconds = 0;
      emit endpointsChanged(edgeU, -1);
    }
    emit routeChanged();
  }

  void handleRightClick(QMouseEvent *e) {
    QPointF sp = mapToScene(e->pos());
    auto [u, v, proj] = findNearestPointOnEdge(sp);
    QMenu menu(this);

    if (u != -1) {
      double cur = trafficOf(graph, u, v);
      menu.addSection(QString("%1 ↔ %2 (mật độ %3)")
                          .arg(graph.nodes[u].name, graph.nodes[v].name)
                          .arg(cur, 0, 'f', 1));
      QAction *a1 = menu.addAction("Đặt mật độ 1.0 (thông)");
      QAction *a2 = menu.addAction("Đặt mật độ 1.5 (đông vừa)");
      QAction *a3 = menu.addAction("Đặt mật độ 2.0 (tắc vừa)");
      QAction *a4 = menu.addAction("Đặt mật độ 2.5 (tắc nặng)");
      menu.addSeparator();
      QAction *aBan = menu.addAction("Cấm đoạn đường này");

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      QAction *chosen = menu.exec(e->globalPosition().toPoint());
#else
      QAction *chosen = menu.exec(e->globalPos());
#endif
      if (chosen == a1 || chosen == a2 || chosen == a3 || chosen == a4) {
        double newT = (chosen == a1)   ? 1.0
                      : (chosen == a2) ? 1.5
                      : (chosen == a3) ? 2.0
                                       : 2.5;
        for (auto &ed : graph.adj[u])
          if (ed.to == v)
            ed.trafficFactor = newT;
        for (auto &ed : graph.adj[v])
          if (ed.to == u)
            ed.trafficFactor = newT;

        stopAnimation();
        stopPathAnim();

        pathItems.clear();
        markerItems.clear();
        altPathItems.clear();
        secondaryPathItems.clear();
        pathAnimLines.clear();
        gridGroup = nullptr;

        scene->clear();
        drawMap();
        redrawMarkers();
        recompute();
      } else if (chosen == aBan) {
        bannedEdges.insert({u, v});
        recompute();
      }
      return;
    }

    QAction *aClr = menu.addAction("Xoá tất cả (Esc)");
    QAction *aSwap = menu.addAction("Đảo A ↔ B");
    menu.addSeparator();
    QAction *aUnban = menu.addAction("Bỏ cấm tất cả");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAction *chosen = menu.exec(e->globalPosition().toPoint());
#else
    QAction *chosen = menu.exec(e->globalPos());
#endif
    if (chosen == aClr)
      clearAll();
    else if (chosen == aSwap)
      swapAB();
    else if (chosen == aUnban) {
      bannedEdges.clear();
      bannedNodes.clear();
      recompute();
    }
  }

  void computeRouteInfo(const Graph &g, const QVector<int> &path,
                        QVector<RouteStep> &steps, double &outMeters,
                        int &outSeconds) {
    steps.clear();
    outMeters = 0;
    outSeconds = 0;

    for (int i = 0; i < path.size() - 1; ++i) {
      int u = path[i], v = path[i + 1];
      double traf = trafficOf(g, u, v);
      double effT = traf * peakMul;
      QColor c = trafficToColor(traf);
      QPointF p1 = g.nodes[u].pos, p2 = g.nodes[v].pos;
      double distPx = QLineF(p1, p2).length();
      double h = Graph::travelTimeOf(distPx, effT, v0);

      RouteStep s;
      s.from = g.nodes[u].name;
      s.to = g.nodes[v].name;
      s.meters = distPx * 0.005 * 1000.0;
      s.seconds = static_cast<int>(h * 3600);
      s.traffic = traf;
      s.color = c;
      s.street = streetOf(g, u, v);

      bool bridge = isBridgeOf(g, u, v);
      if (bridge) {
        s.instruction = QString("Qua %1").arg(s.street.isEmpty() ? s.to : s.street);
        s.icon = "🌉";
      } else if (i == 0) {
        s.instruction = QString("Khởi hành từ %1, đi vào %2")
                            .arg(s.from, s.street.isEmpty() ? s.to : s.street);
        s.icon = "🏁";
      } else {
        QPointF p0 = g.nodes[path[i - 1]].pos;
        QPointF v1 = p1 - p0;
        QPointF v2 = p2 - p1;
        double cross = v1.x() * v2.y() - v1.y() * v2.x();
        double dot = v1.x() * v2.x() + v1.y() * v2.y();
        double l1 = std::hypot(v1.x(), v1.y());
        double l2 = std::hypot(v2.x(), v2.y());
        double angleRad = std::acos(std::clamp(dot / (l1 * l2 + 1e-6), -1.0, 1.0));
        double angleDeg = angleRad * 180.0 / 3.14159265;

        if (angleDeg < 25.0) {
          s.instruction = QString("Đi thẳng tiếp vào %1").arg(s.street.isEmpty() ? s.to : s.street);
          s.icon = "⬆️";
        } else if (cross > 0) {
          s.instruction = QString("Rẽ phải vào %1").arg(s.street.isEmpty() ? s.to : s.street);
          s.icon = "↱";
        } else {
          s.instruction = QString("Rẽ trái vào %1").arg(s.street.isEmpty() ? s.to : s.street);
          s.icon = "↰";
        }
      }

      steps.append(s);
      outMeters += s.meters;
      outSeconds += s.seconds;
    }
  }

  void drawRoute(const Graph &g, const QVector<int> &path,
                 QVector<QGraphicsItem *> &items, bool isPrimary,
                 QColor overrideColor = QColor()) {
    if (isPrimary)
      pathAnimLines.clear();

    for (int i = 0; i < path.size() - 1; ++i) {
      int u = path[i], v = path[i + 1];
      QPointF p1 = g.nodes[u].pos, p2 = g.nodes[v].pos;

      if (isPrimary) {
        double traf = trafficOf(g, u, v);
        double effT = traf * peakMul;
        // Hiển thị màu sắc mật độ giao thông (Xanh/Vàng/Cam/Đỏ) khi bật, hoặc màu xanh Google Maps chuẩn khi tắt
        QColor c = showTrafficLayer ? trafficToColor(traf) : QColor(26, 115, 232);
        int w = 8 + (effT >= 1.8 ? 2 : 0) + (effT >= 2.2 ? 2 : 0);

        auto *shadow = scene->addLine(QLineF(p1, p2),
                                      QPen(darkMode ? QColor(30, 30, 35) : Qt::white, w + 4,
                                           Qt::SolidLine, Qt::RoundCap,
                                           Qt::RoundJoin));
        shadow->setZValue(18);
        items.append(shadow);

        auto *line = scene->addLine(QLineF(p1, p2),
                                    QPen(c, w, Qt::SolidLine,
                                         Qt::RoundCap, Qt::RoundJoin));
        line->setZValue(20);
        items.append(line);
        pathAnimLines.append(static_cast<QGraphicsLineItem *>(line));

        double len = QLineF(p1, p2).length();
        if (len > 35.0) {
          QPointF dir = (p2 - p1) / len;
          QPointF norm(-dir.y(), dir.x());
          int steps = static_cast<int>(len / 45.0);
          for (int s = 1; s <= steps; ++s) {
            double t = static_cast<double>(s) / (steps + 1);
            QPointF pt = p1 + t * (p2 - p1);

            QPainterPath arr;
            arr.moveTo(pt - dir * 4.0 + norm * 3.5);
            arr.lineTo(pt + dir * 3.0);
            arr.lineTo(pt - dir * 4.0 - norm * 3.5);

            auto *arrItem = scene->addPath(arr, QPen(QColor(255, 255, 255, 220), 2.0,
                                                     Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            arrItem->setZValue(21);
            items.append(arrItem);
          }
        }
      } else {
        QPen pen(overrideColor, 6, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        auto *line = scene->addLine(QLineF(p1, p2), pen);
        line->setZValue(17);
        items.append(line);
      }
    }
  }

  void recompute() {
    if (startPoint.isNull() || endPoint.isNull())
      return;
    clearPath();
    stopAnimation();
    stopPathAnim();
    lastSteps.clear();
    totalMeters = 0;
    totalSeconds = 0;
    altPaths.clear();
    timeSteps.clear();
    distSteps.clear();
    timeTotalMeters = distTotalMeters = 0;
    timeTotalSeconds = distTotalSeconds = 0;
    dualRoutesAvailable = false;
    dualRoutesSame = false;
    primaryPoints.clear();

    QSet<QPair<int, int>> localBanned = bannedEdges;
    if (avoidCongested) {
      for (int u = 0; u < graph.nodes.size(); ++u)
        for (const Edge &e : graph.adj[u])
          if (u < e.to && e.trafficFactor * peakMul >= 2.0)
            localBanned.insert({u, e.to});
    }

    bool sameEdge = (startEdgeU == endEdgeU && startEdgeV == endEdgeV) ||
                    (startEdgeU == endEdgeV && startEdgeV == endEdgeU);
    if (sameEdge && startEdgeV != -1) {
      double traf = trafficOf(graph, startEdgeU, startEdgeV) * peakMul;
      QColor c = showTrafficLayer ? trafficToColor(traf / peakMul) : QColor(26, 115, 232);
      QPen pen(c, 10, Qt::SolidLine, Qt::RoundCap);
      auto *line = scene->addLine(QLineF(startPoint, endPoint), pen);
      line->setZValue(20);
      pathItems.append(line);

      double distPx = QLineF(startPoint, endPoint).length();
      double h = Graph::travelTimeOf(distPx, traf, v0);
      totalMeters = distPx * 0.005 * 1000.0;
      totalSeconds = static_cast<int>(h * 3600);

      RouteStep s{"A", "B", totalMeters, totalSeconds, traf / peakMul,
                  pen.color(), "Đi thẳng đến đích", "", "🏁"};
      lastSteps.append(s);
      primaryPoints = {startPoint, endPoint};
      emit routeChanged();
      emit statusMessage(QString("Đi thẳng: %1 • %2s")
                             .arg(fmtDist(totalMeters))
                             .arg(totalSeconds));
      startAnimation(primaryPoints);
      return;
    }

    Graph temp = graph;
    int startId = temp.nodes.size();
    int endId = startId + 1;
    temp.addNode(startId, "A", startPoint);
    temp.addNode(endId, "B", endPoint);

    auto attach = [&](int id, QPointF pt, int eu, int ev) {
      if (ev == -1) {
        temp.addEdge(id, eu, 0.01, 1.0);
      } else {
        double t = trafficOf(graph, eu, ev);
        temp.addEdge(id, eu, QLineF(pt, temp.nodes[eu].pos).length(), t);
        temp.addEdge(id, ev, QLineF(pt, temp.nodes[ev].pos).length(), t);
      }
    };
    attach(startId, startPoint, startEdgeU, startEdgeV);
    attach(endId, endPoint, endEdgeU, endEdgeV);

    QVector<int> timePath =
        temp.dijkstra(startId, endId, bannedNodes, localBanned, peakMul, v0);
    QVector<int> distPath =
        temp.dijkstraByDistance(startId, endId, bannedNodes, localBanned);

    if (timePath.size() < 2 && distPath.size() < 2) {
      emit statusMessage("Không tìm thấy đường đi!");
      emit routeChanged();
      return;
    }

    if (timePath.size() >= 2)
      computeRouteInfo(temp, timePath, timeSteps, timeTotalMeters,
                       timeTotalSeconds);
    if (distPath.size() >= 2)
      computeRouteInfo(temp, distPath, distSteps, distTotalMeters,
                       distTotalSeconds);

    dualRoutesSame = (timePath == distPath);
    dualRoutesAvailable = (timePath.size() >= 2 && distPath.size() >= 2);

    QVector<int> *primaryPtr = nullptr;
    QVector<int> *secondaryPtr = nullptr;

    if (routeMode == DistRoute && distPath.size() >= 2) {
      primaryPtr = &distPath;
      secondaryPtr = &timePath;
    } else {
      primaryPtr = &timePath;
      secondaryPtr = &distPath;
    }

    if (primaryPtr->size() < 2 && secondaryPtr->size() >= 2)
      std::swap(primaryPtr, secondaryPtr);

    QVector<int> &primaryPath = *primaryPtr;
    QVector<int> &secondaryPath = *secondaryPtr;

    bool drawSecondary = dualRoutesAvailable && !dualRoutesSame &&
                         secondaryPath.size() >= 2 && primaryPath.size() >= 2;

    if (drawSecondary) {
      QColor secColor =
          (routeMode == DistRoute) ? QColor(255, 140, 0) : QColor(100, 150, 200);
      drawRoute(temp, secondaryPath, secondaryPathItems, false, secColor);
    }

    for (int i = 0; i < primaryPath.size(); ++i)
      primaryPoints.append(temp.nodes[primaryPath[i]].pos);

    drawRoute(temp, primaryPath, pathItems, true);

    if (routeMode == DistRoute && distPath.size() >= 2) {
      lastSteps = distSteps;
      totalMeters = distTotalMeters;
      totalSeconds = distTotalSeconds;
    } else {
      lastSteps = timeSteps;
      totalMeters = timeTotalMeters;
      totalSeconds = timeTotalSeconds;
    }

    if (showAlternatives) {
      QVector<QPair<int, int>> mainEdges;
      for (int i = 0; i < primaryPath.size() - 1; ++i) {
        int u = primaryPath[i], v = primaryPath[i + 1];
        if (u < startId && v < startId)
          mainEdges.append({qMin(u, v), qMax(u, v)});
      }
      for (int k = 0; k < qMin(2, mainEdges.size()); ++k) {
        QSet<QPair<int, int>> extra = localBanned;
        extra.insert(mainEdges[k]);
        QVector<int> alt =
            temp.dijkstra(startId, endId, bannedNodes, extra, peakMul, v0);
        if (alt.size() >= 2) {
          QVector<QPointF> ap;
          QColor ac = (k == 0) ? QColor(0, 150, 255) : QColor(255, 140, 0);
          for (int i = 0; i < alt.size() - 1; ++i) {
            auto *line = scene->addLine(
                QLineF(temp.nodes[alt[i]].pos, temp.nodes[alt[i + 1]].pos),
                QPen(ac, 6, Qt::DashLine));
            line->setZValue(18);
            altPathItems.append(line);
            ap.append(temp.nodes[alt[i]].pos);
          }
          ap.append(temp.nodes[alt.last()].pos);
          altPaths.append(ap);
        }
      }
    }

    emit routeChanged();
    QString modeStr = (routeMode == DistRoute) ? "Quãng đường ngắn nhất"
                                               : "Thời gian nhanh nhất";
    QString msg = QString("[%1] %2 • %3 ph %4 s")
                      .arg(modeStr)
                      .arg(fmtDist(totalMeters))
                      .arg(totalSeconds / 60)
                      .arg(totalSeconds % 60);
    emit statusMessage(msg);

    updateRouteBanner();
    startAnimation(primaryPoints);
    startPathAnimation();
  }

  QString fmtDist(double meters) const {
    if (useMeters)
      return QString("%1 m").arg(meters, 0, 'f', 0);
    return QString("%1 km").arg(meters / 1000.0, 0, 'f', 2);
  }

  void startAnimation(const QVector<QPointF> &pts) {
    stopAnimation();
    if (pts.size() < 2)
      return;

    animSegments.clear();
    animTotalDist = 0.0;
    for (int i = 0; i < pts.size() - 1; ++i) {
      QPointF p0 = pts[i], p1 = pts[i + 1];
      double len = QLineF(p0, p1).length();
      if (len < 1e-4) continue;
      double angle = std::atan2(p1.y() - p0.y(), p1.x() - p0.x()) * 180.0 / 3.14159265;
      animTotalDist += len;
      animSegments.append({p0, p1, len, animTotalDist, angle});
    }
    if (animSegments.isEmpty()) return;

    animCurrentDist = 0.0;
    animSpeed = (transportMode == Car) ? 190.0 : ((transportMode == Motorbike) ? 150.0 : 70.0);

    mover = scene->createItemGroup({});

    // Hào quang tỏa sáng xung quanh xe
    auto *outerHalo = scene->addEllipse(-18, -18, 36, 36,
                                        QPen(Qt::NoPen),
                                        QBrush(QColor(66, 133, 244, 55)));
    mover->addToGroup(outerHalo);

    // Vòng tròn nền
    auto *whiteRing = scene->addEllipse(-12, -12, 24, 24,
                                        QPen(Qt::white, 2.0),
                                        QBrush(QColor(26, 115, 232)));
    mover->addToGroup(whiteRing);

    if (transportMode == Car) {
      // Tia đèn pha chiếu sáng phía trước
      QPolygonF headlight;
      headlight << QPointF(-4, -8) << QPointF(-14, -28) << QPointF(14, -28) << QPointF(4, -8);
      auto *hlItem = scene->addPolygon(headlight, QPen(Qt::NoPen), QBrush(QColor(255, 238, 88, 80)));
      mover->addToGroup(hlItem);

      // Thân xe ô tô sedan
      QPainterPath carPath;
      carPath.addRoundedRect(QRectF(-6, -9, 12, 18), 3, 3);
      auto *carBody = scene->addPath(carPath, QPen(Qt::white, 1), QBrush(QColor(245, 245, 255)));
      mover->addToGroup(carBody);

      // Kính chắn gió
      auto *windshield = scene->addRect(-4, -4, 8, 4, QPen(Qt::NoPen), QBrush(QColor(40, 50, 70)));
      mover->addToGroup(windshield);

      // Đèn hậu đỏ
      auto *tl1 = scene->addRect(-5, 7, 3, 2, QPen(Qt::NoPen), QBrush(QColor(234, 67, 53)));
      auto *tl2 = scene->addRect(2, 7, 3, 2, QPen(Qt::NoPen), QBrush(QColor(234, 67, 53)));
      mover->addToGroup(tl1);
      mover->addToGroup(tl2);
    } else if (transportMode == Motorbike) {
      // Đèn xe máy
      QPolygonF headlight;
      headlight << QPointF(0, -6) << QPointF(-8, -22) << QPointF(8, -22);
      auto *hlItem = scene->addPolygon(headlight, QPen(Qt::NoPen), QBrush(QColor(255, 238, 88, 90)));
      mover->addToGroup(hlItem);

      // Thân xe máy + người lái
      auto *bikeBody = scene->addRect(-2, -7, 4, 14, QPen(Qt::white, 1), QBrush(QColor(251, 188, 4)));
      mover->addToGroup(bikeBody);

      auto *helmet = scene->addEllipse(-3.5, -2, 7, 7, QPen(Qt::NoPen), QBrush(QColor(234, 67, 53)));
      mover->addToGroup(helmet);
    } else {
      // Đi bộ (Walker)
      QPolygonF arrow;
      arrow << QPointF(0, -7) << QPointF(-4, 3) << QPointF(0, 1) << QPointF(4, 3);
      auto *arrowItem = scene->addPolygon(arrow, QPen(Qt::NoPen), QBrush(Qt::white));
      mover->addToGroup(arrowItem);
    }

    mover->setZValue(42);
    mover->setPos(pts[0]);
    mover->setRotation(animSegments[0].angleDeg + 90.0);

    smoothAnimTimer->start(35);
  }

  void onAnimTick() {
    // Để tương thích ngược
  }

  void stopAnimation() {
    if (animTimer)
      animTimer->stop();
    if (smoothAnimTimer)
      smoothAnimTimer->stop();
    if (mover) {
      if (mover->scene())
        scene->removeItem(mover);
      delete mover;
      mover = nullptr;
    }
    animPath.clear();
    animSegments.clear();
    animIndex = 0;
    animCurrentDist = 0.0;
  }

  void startPathAnimation() {
    pathAnimIndex = 0;
    for (auto *l : pathAnimLines)
      if (l && l->scene())
        l->setVisible(false);
    pathAnimTimer->start(70);
  }

  void onPathAnimTick() {
    if (pathAnimIndex >= pathAnimLines.size()) {
      pathAnimTimer->stop();
      return;
    }
    auto *line = pathAnimLines[pathAnimIndex++];
    if (line && line->scene())
      line->setVisible(true);
  }

  void stopPathAnim() {
    if (pathAnimTimer)
      pathAnimTimer->stop();
    pathAnimIndex = 0;
  }

  void onTrafficSimTick() {
    simMinute += 20;
    bool hourChanged = false;
    if (simMinute >= 60) {
      simMinute = 0;
      simHour++;
      if (simHour > 22)
        simHour = 6;
      hourChanged = true;
    }

    QString periodDesc;
    double minF = 0.8, maxF = 1.2;

    if (simHour >= 7 && simHour <= 9) {
      periodDesc = "Cao điểm sáng";
      minF = 1.3;
      maxF = 2.0;
    } else if (simHour >= 11 && simHour <= 13) {
      periodDesc = "Trưa";
      minF = 1.0;
      maxF = 1.3;
    } else if (simHour >= 17 && simHour <= 19) {
      periodDesc = "Cao điểm chiều";
      minF = 1.5;
      maxF = 2.2;
    } else {
      periodDesc = "Bình thường";
      minF = 0.8;
      maxF = 1.2;
    }

    // Cập nhật trafficFactor cho ~30% cạnh ngẫu nhiên khi đổi giờ
    if (hourChanged || primaryPoints.isEmpty()) {
      for (int u = 0; u < graph.nodes.size(); ++u) {
        for (int i = 0; i < graph.adj[u].size(); ++i) {
          int v = graph.adj[u][i].to;
          if (u < v) {
            if ((std::rand() % 100) < 30) {
              double rnd = static_cast<double>(std::rand()) / RAND_MAX;
              double factor = minF + rnd * (maxF - minF);
              graph.adj[u][i].trafficFactor = factor;
              for (int j = 0; j < graph.adj[v].size(); ++j) {
                if (graph.adj[v][j].to == u) {
                  graph.adj[v][j].trafficFactor = factor;
                  break;
                }
              }
            }
          }
        }
      }
      recompute();
      viewport()->update();
    }

    QString timeStr = QString("%1:%2")
                          .arg(simHour, 2, 10, QChar('0'))
                          .arg(simMinute, 2, 10, QChar('0'));
    emit trafficSimTimeChanged(simHour, QString("%1 – %2").arg(timeStr, periodDesc));
    emit statusMessage(QString("Mô phỏng GT: 🕐 %1 – %2").arg(timeStr, periodDesc));
  }
};

// ==================== GIAO DIỆN CHÍNH MAIN WINDOW ====================
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow() {
    setWindowTitle("Bản đồ Đà Nẵng — Dẫn đường thông minh Google Maps");
    resize(1460, 900);

    setStyleSheet(R"(
      QMainWindow { background: #F8F9FA; }
      QToolBar {
        background: #FFFFFF;
        border-bottom: 1px solid #DADCE0;
        spacing: 6px;
        padding: 4px 8px;
      }
      QToolBar QToolButton {
        color: #3C4043;
        font-family: 'Segoe UI';
        font-size: 12px;
        font-weight: 500;
        padding: 5px 10px;
        border-radius: 4px;
        border: none;
        background: transparent;
      }
      QToolBar QToolButton:hover  { background: #F1F3F4; }
      QToolBar QToolButton:pressed { background: #E8EAED; }
      QToolBar QLabel {
        color: #5F6368;
        font-family: 'Segoe UI';
        font-size: 12px;
        font-weight: 500;
      }
      QToolBar QCheckBox {
        color: #3C4043;
        font-family: 'Segoe UI';
        font-size: 12px;
        spacing: 6px;
      }
      QToolBar QCheckBox::indicator {
        width: 15px; height: 15px;
        border: 2px solid #9AA0A6;
        border-radius: 3px;
        background: white;
      }
      QToolBar QCheckBox::indicator:checked {
        background: #4285F4;
        border-color: #4285F4;
      }
      QDockWidget {
        color: #202124;
        font-family: 'Segoe UI';
        font-size: 13px;
      }
      QDockWidget::title {
        background: #FFFFFF;
        padding: 10px 14px;
        border-bottom: 1px solid #DADCE0;
        font-weight: bold;
        font-size: 14px;
        color: #202124;
      }
      QListWidget {
        background: #FFFFFF;
        border: none;
        font-family: 'Segoe UI';
        font-size: 12px;
        color: #3C4043;
        outline: none;
      }
      QListWidget::item {
        padding: 8px 10px;
        border-bottom: 1px solid #F1F3F4;
        min-height: 36px;
      }
      QListWidget::item:hover   { background: #F8F9FA; }
      QListWidget::item:selected { background: #E8F0FE; color: #1A73E8; }
      QComboBox {
        border: 1px solid #DADCE0;
        border-radius: 6px;
        padding: 5px 12px;
        background: #FFFFFF;
        font-family: 'Segoe UI';
        font-size: 12px;
        color: #3C4043;
        min-height: 24px;
      }
      QComboBox:hover    { border-color: #4285F4; }
      QComboBox:focus    { border: 2px solid #4285F4; }
      QComboBox::drop-down { border: none; width: 22px; }
      QStatusBar {
        background: #FFFFFF;
        color: #5F6368;
        font-family: 'Segoe UI';
        font-size: 11px;
        border-top: 1px solid #DADCE0;
      }
      QSlider::groove:horizontal {
        height: 4px;
        background: #DADCE0;
        border-radius: 2px;
      }
      QSlider::handle:horizontal {
        background: #4285F4;
        width: 14px; height: 14px;
        margin: -5px 0;
        border-radius: 7px;
        border: none;
      }
      QSlider::sub-page:horizontal {
        background: #4285F4;
        border-radius: 2px;
      }
    )");

    view = new MapView(this);
    setCentralWidget(view);

    buildTopToolbar();
    buildDirectionsDock();
    buildBottomToolbar();

    connect(view, &MapView::routeChanged, this, &MainWindow::onRouteUpdated);
    connect(view, &MapView::statusMessage, this,
            [this](const QString &m) { statusBar()->showMessage(m, 4000); });
    connect(view, &MapView::mousePosChanged, this, [this](QPointF p) {
      coordLabel->setText(
          QString("(%1, %2)").arg(p.x(), 0, 'f', 0).arg(p.y(), 0, 'f', 0));
    });
    connect(view, &MapView::endpointsChanged, this, &MainWindow::onEndpointsUpdated);

    statusBar()->showMessage(
        "Sẵn sàng — Bản đồ đầy đủ 81 giao lộ Đà Nẵng | Click chọn điểm A → B");
    onRouteUpdated();
    view->setEndpoints(5, 55);
  }

private slots:
  void onEndpointsUpdated(int startId, int endId) {
    disconnect(startCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    disconnect(destCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);

    startCombo->setCurrentIndex(startId >= 0 ? startId + 1 : 0);
    destCombo->setCurrentIndex(endId >= 0 ? endId + 1 : 0);

    connect(startCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    connect(destCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
  }

  void onComboChanged(int) {
    int sIdx = startCombo->currentIndex() - 1;
    int dIdx = destCombo->currentIndex() - 1;
    if (sIdx >= 0 && dIdx >= 0 && sIdx != dIdx) {
      view->setEndpoints(sIdx, dIdx);
    } else if (sIdx >= 0 && dIdx < 0) {
      view->setEndpoints(sIdx, sIdx);
    }
  }

  void onRouteUpdated() {
    if (view->lastRoute().isEmpty() && !view->hasDualRoutes()) {
      etaLabel->setText("-- phút");
      etaLabel->setStyleSheet("color: #70757A; font-size: 26px; font-weight: bold;");
      distEtaLabel->setText("Chưa có tuyến đường được chọn");
      trafficChip->setText("● Chọn điểm đi & điểm đến");
      trafficChip->setStyleSheet("background: #F1F3F4; color: #5F6368; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
    } else {
      int totalSecs = view->lastTotalSeconds();
      int mins = (totalSecs + 30) / 60;
      if (mins < 1) mins = 1;
      etaLabel->setText(QString("%1 phút").arg(mins));

      QTime etaTime = QTime::currentTime().addSecs(totalSecs);
      distEtaLabel->setText(QString("%1 • Dự kiến đến %2")
                                .arg(unitStr(view->lastTotalMeters()))
                                .arg(etaTime.toString("HH:mm")));

      bool congested = false;
      for (const auto &s : view->lastRoute()) {
        if (s.traffic >= 1.8) {
          congested = true;
          break;
        }
      }
      if (congested) {
        etaLabel->setStyleSheet("color: #D93025; font-size: 28px; font-weight: bold;");
        trafficChip->setText("● Có đoạn ùn ứ");
        trafficChip->setStyleSheet("background: #FCE8E6; color: #C5221F; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
      } else {
        etaLabel->setStyleSheet("color: #188038; font-size: 28px; font-weight: bold;");
        trafficChip->setText("● Tuyến nhanh nhất • Đường thông thoáng");
        trafficChip->setStyleSheet("background: #E6F4EA; color: #137333; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
      }
    }

    if (view->hasDualRoutes() && !view->areDualRoutesSame()) {
      dualRouteWidget->setVisible(true);
      int tSec = view->timeRouteTotalSeconds();
      int dSec = view->distRouteTotalSeconds();
      timeRouteBtn->setText(QString("⏱ Nhanh nhất: %1 ph (%2)")
                               .arg((tSec + 30) / 60)
                               .arg(unitStr(view->timeRouteTotalMeters())));
      distRouteBtn->setText(QString("📏 Ngắn nhất: %1 ph (%2)")
                               .arg((dSec + 30) / 60)
                               .arg(unitStr(view->distRouteTotalMeters())));

      if (view->currentRouteMode() == MapView::TimeRoute) {
        timeRouteBtn->setStyleSheet("QPushButton { background: #E8F0FE; color: #1A73E8; border: 1.5px solid #1A73E8; border-radius: 6px; font-weight: bold; padding: 6px; }");
        distRouteBtn->setStyleSheet("QPushButton { background: #FFFFFF; color: #5F6368; border: 1px solid #DADCE0; border-radius: 6px; padding: 6px; }");
      } else {
        distRouteBtn->setStyleSheet("QPushButton { background: #E8F0FE; color: #1A73E8; border: 1.5px solid #1A73E8; border-radius: 6px; font-weight: bold; padding: 6px; }");
        timeRouteBtn->setStyleSheet("QPushButton { background: #FFFFFF; color: #5F6368; border: 1px solid #DADCE0; border-radius: 6px; padding: 6px; }");
      }
    } else {
      dualRouteWidget->setVisible(false);
    }

    stepList->clear();
    const auto &steps = view->lastRoute();
    if (steps.isEmpty()) {
      auto *emptyItem = new QListWidgetItem("Hãy nhấp chọn điểm A và B trên bản đồ");
      emptyItem->setTextAlignment(Qt::AlignCenter);
      emptyItem->setForeground(QColor(128, 134, 139));
      stepList->addItem(emptyItem);
    } else {
      for (int i = 0; i < steps.size(); ++i) {
        const auto &s = steps[i];
        QString txt = QString("%1  %2\n     %3 • %4 giây")
                          .arg(s.icon, s.instruction)
                          .arg(unitStr(s.meters))
                          .arg(s.seconds);
        auto *it = new QListWidgetItem(txt);
        QPixmap pm(10, 10);
        pm.fill(s.color);
        it->setIcon(QIcon(pm));
        stepList->addItem(it);
      }

      auto *finish = new QListWidgetItem(QString("🏁  Đến điểm hẹn: %1")
                                             .arg(steps.last().to));
      QFont ff = finish->font();
      ff.setBold(true);
      finish->setFont(ff);
      finish->setForeground(QColor(234, 67, 53));
      stepList->addItem(finish);
    }

    statsLabel->setText(QString("Giao lộ: %1 | Tuyến đường: %2")
                            .arg(view->nodeCount())
                            .arg(view->edgeCount()));
  }

  void copyRoute() {
    const auto &steps = view->lastRoute();
    if (steps.isEmpty())
      return;
    QString t = "===== LỘ TRÌNH GOOGLE MAPS ĐÀ NẴNG =====\n";
    for (int i = 0; i < steps.size(); ++i) {
      t += QString("%1. %2 (%3, %4s, mật độ %5)\n")
               .arg(i + 1)
               .arg(steps[i].instruction)
               .arg(unitStr(steps[i].meters))
               .arg(steps[i].seconds)
               .arg(steps[i].traffic, 0, 'f', 1);
    }
    t += QString("TỔNG CỘNG: %1 | %2 phút %3 giây\n")
             .arg(unitStr(view->lastTotalMeters()))
             .arg(view->lastTotalSeconds() / 60)
             .arg(view->lastTotalSeconds() % 60);
    QApplication::clipboard()->setText(t);
    statusBar()->showMessage("Đã sao chép lộ trình chỉ đường", 2500);
  }

  void savePng() {
    QString fn = QFileDialog::getSaveFileName(this, "Lưu ảnh bản đồ",
                                              "GoogleMaps_DaNang_Full.png", "PNG (*.png)");
    if (fn.isEmpty())
      return;
    if (!fn.endsWith(".png", Qt::CaseInsensitive))
      fn += ".png";
    view->savePng(fn);
  }

  void showNodeTable() {
    QDialog dlg(this);
    dlg.setWindowTitle("Danh mục các giao lộ & địa điểm Đà Nẵng");
    dlg.resize(560, 480);
    dlg.setStyleSheet("background: #FFFFFF; font-family: 'Segoe UI';");

    auto *tw = new QTableWidget(&dlg);
    tw->setColumnCount(4);
    tw->setHorizontalHeaderLabels({"ID", "Địa điểm / Tuyến đường", "Tọa độ", "Số nhánh"});
    tw->setRowCount(view->nodeCount());
    auto names = view->nodeNames();
    for (int i = 0; i < names.size(); ++i) {
      tw->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
      tw->setItem(i, 1, new QTableWidgetItem(names[i]));
      QPointF p = view->nodePos(i);
      tw->setItem(
          i, 2,
          new QTableWidgetItem(
              QString("(%1, %2)").arg(p.x(), 0, 'f', 0).arg(p.y(), 0, 'f', 0)));
      tw->setItem(i, 3,
                  new QTableWidgetItem(QString::number(view->nodeDegree(i))));
    }
    tw->horizontalHeader()->setStretchLastSection(true);
    tw->verticalHeader()->setVisible(false);
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    lay->addWidget(tw);
    dlg.exec();
  }

private:
  MapView *view = nullptr;
  QListWidget *stepList = nullptr;
  QLabel *coordLabel = nullptr;
  QLabel *statsLabel = nullptr;

  QComboBox *startCombo = nullptr;
  QComboBox *destCombo = nullptr;
  QLabel *etaLabel = nullptr;
  QLabel *distEtaLabel = nullptr;
  QLabel *trafficChip = nullptr;
  QLabel *simTimeLabel = nullptr;
  QWidget *dualRouteWidget = nullptr;
  QPushButton *timeRouteBtn = nullptr;
  QPushButton *distRouteBtn = nullptr;

  QWidget *dockContentWidget = nullptr;
  QFrame *planCardWidget = nullptr;
  QFrame *etaCardWidget = nullptr;
  QLabel *stepsTitleLabel = nullptr;

  bool useMetersFlag = true;

  QString unitStr(double m) const {
    if (useMetersFlag)
      return QString("%1 m").arg(m, 0, 'f', 0);
    return QString("%1 km").arg(m / 1000.0, 0, 'f', 2);
  }

  // ── Dark / Light theme sync ──────────────────────────────────────
  void applyAppTheme(bool dark) {
    // ── Main window / toolbar / dock stylesheet ──
    if (dark) {
      setStyleSheet(R"(
        QMainWindow { background: #1A1A2E; }
        QToolBar {
          background: #16213E;
          border-bottom: 1px solid #0F3460;
          spacing: 6px;
          padding: 4px 8px;
        }
        QToolBar QToolButton {
          color: #E0E0E0;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
          padding: 5px 10px;
          border-radius: 4px;
          border: none;
          background: transparent;
        }
        QToolBar QToolButton:hover   { background: #0F3460; }
        QToolBar QToolButton:pressed { background: #533483; }
        QToolBar QLabel {
          color: #A8B2C1;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
        }
        QToolBar QCheckBox {
          color: #C8D0DC;
          font-family: 'Segoe UI';
          font-size: 12px;
          spacing: 6px;
        }
        QToolBar QCheckBox::indicator {
          width: 15px; height: 15px;
          border: 2px solid #555E6D;
          border-radius: 3px;
          background: #2A2D3E;
        }
        QToolBar QCheckBox::indicator:checked {
          background: #4285F4;
          border-color: #4285F4;
        }
        QDockWidget {
          color: #E0E0E0;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QDockWidget::title {
          background: #16213E;
          padding: 10px 14px;
          border-bottom: 1px solid #0F3460;
          font-weight: bold;
          font-size: 14px;
          color: #FFFFFF;
        }
        QListWidget {
          background: #1E2235;
          border: none;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #C8D0DC;
          outline: none;
        }
        QListWidget::item {
          padding: 8px 10px;
          border-bottom: 1px solid #2A2D3E;
          min-height: 36px;
        }
        QListWidget::item:hover    { background: #252840; }
        QListWidget::item:selected { background: #1A3A5C; color: #7AB3FF; }
        QComboBox {
          border: 1px solid #3A4560;
          border-radius: 6px;
          padding: 5px 12px;
          background: #1E2235;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #C8D0DC;
          min-height: 24px;
        }
        QComboBox:hover  { border-color: #4285F4; }
        QComboBox:focus  { border: 2px solid #4285F4; }
        QComboBox::drop-down { border: none; width: 22px; }
        QComboBox QAbstractItemView {
          background: #1E2235;
          color: #C8D0DC;
          selection-background-color: #1A3A5C;
        }
        QStatusBar {
          background: #16213E;
          color: #8892A4;
          font-family: 'Segoe UI';
          font-size: 11px;
          border-top: 1px solid #0F3460;
        }
        QSlider::groove:horizontal {
          height: 4px;
          background: #3A4560;
          border-radius: 2px;
        }
        QSlider::handle:horizontal {
          background: #4285F4;
          width: 14px; height: 14px;
          margin: -5px 0;
          border-radius: 7px;
          border: none;
        }
        QSlider::sub-page:horizontal {
          background: #4285F4;
          border-radius: 2px;
        }
      )");
    } else {
      setStyleSheet(R"(
        QMainWindow { background: #F8F9FA; }
        QToolBar {
          background: #FFFFFF;
          border-bottom: 1px solid #DADCE0;
          spacing: 6px;
          padding: 4px 8px;
        }
        QToolBar QToolButton {
          color: #3C4043;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
          padding: 5px 10px;
          border-radius: 4px;
          border: none;
          background: transparent;
        }
        QToolBar QToolButton:hover  { background: #F1F3F4; }
        QToolBar QToolButton:pressed { background: #E8EAED; }
        QToolBar QLabel {
          color: #5F6368;
          font-family: 'Segoe UI';
          font-size: 12px;
          font-weight: 500;
        }
        QToolBar QCheckBox {
          color: #3C4043;
          font-family: 'Segoe UI';
          font-size: 12px;
          spacing: 6px;
        }
        QToolBar QCheckBox::indicator {
          width: 15px; height: 15px;
          border: 2px solid #9AA0A6;
          border-radius: 3px;
          background: white;
        }
        QToolBar QCheckBox::indicator:checked {
          background: #4285F4;
          border-color: #4285F4;
        }
        QDockWidget {
          color: #202124;
          font-family: 'Segoe UI';
          font-size: 13px;
        }
        QDockWidget::title {
          background: #FFFFFF;
          padding: 10px 14px;
          border-bottom: 1px solid #DADCE0;
          font-weight: bold;
          font-size: 14px;
          color: #202124;
        }
        QListWidget {
          background: #FFFFFF;
          border: none;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #3C4043;
          outline: none;
        }
        QListWidget::item {
          padding: 8px 10px;
          border-bottom: 1px solid #F1F3F4;
          min-height: 36px;
        }
        QListWidget::item:hover   { background: #F8F9FA; }
        QListWidget::item:selected { background: #E8F0FE; color: #1A73E8; }
        QComboBox {
          border: 1px solid #DADCE0;
          border-radius: 6px;
          padding: 5px 12px;
          background: #FFFFFF;
          font-family: 'Segoe UI';
          font-size: 12px;
          color: #3C4043;
          min-height: 24px;
        }
        QComboBox:hover    { border-color: #4285F4; }
        QComboBox:focus    { border: 2px solid #4285F4; }
        QComboBox::drop-down { border: none; width: 22px; }
        QStatusBar {
          background: #FFFFFF;
          color: #5F6368;
          font-family: 'Segoe UI';
          font-size: 11px;
          border-top: 1px solid #DADCE0;
        }
        QSlider::groove:horizontal {
          height: 4px;
          background: #DADCE0;
          border-radius: 2px;
        }
        QSlider::handle:horizontal {
          background: #4285F4;
          width: 14px; height: 14px;
          margin: -5px 0;
          border-radius: 7px;
          border: none;
        }
        QSlider::sub-page:horizontal {
          background: #4285F4;
          border-radius: 2px;
        }
      )");
    }

    // ── Per-widget fine-grained dark style overrides ──
    if (dockContentWidget) {
      dockContentWidget->setStyleSheet(
          dark ? "background: #1A1A2E;" : "background: #FFFFFF;");
    }
    if (planCardWidget) {
      planCardWidget->setStyleSheet(
          dark ? "QFrame { background:#1E2235; border:1px solid #3A4560; border-radius:10px; padding:8px; }"
               : "QFrame { background:#FFFFFF; border:1px solid #DADCE0; border-radius:10px; padding:8px; }");
    }
    if (etaCardWidget) {
      etaCardWidget->setStyleSheet(
          dark ? "QFrame { background:#1E2235; border:1px solid #3A4560; border-radius:10px; padding:10px; }"
               : "QFrame { background:#FFFFFF; border:1px solid #DADCE0; border-radius:10px; padding:10px; }");
    }
    if (stepsTitleLabel) {
      stepsTitleLabel->setStyleSheet(
          dark ? "font-weight:bold; color:#A8B2C1; font-size:13px;"
               : "font-weight:bold; color:#202124; font-size:13px;");
    }
    if (distEtaLabel) {
      distEtaLabel->setStyleSheet(
          dark ? "color:#8892A4; font-size:13px;"
               : "color:#5F6368; font-size:13px;");
    }
    // Refresh the route display so ETA label colour resets properly
    onRouteUpdated();
  }

  void buildTopToolbar() {
    QToolBar *tb = addToolBar("Chính");
    tb->setMovable(false);

    QAction *actClear = tb->addAction("✕ Đặt lại", view, &MapView::clearAll);
    actClear->setToolTip("Xoá điểm chọn hiện tại (Phím Esc)");

    QAction *actSwap = tb->addAction("⇅ Đảo A↔B", view, &MapView::swapAB);
    actSwap->setToolTip("Đảo vị trí điểm đi và điểm đến");

    QAction *actTraffic = tb->addAction("🚦 Mật độ GT", this, [this]() {
      view->setTrafficLayerVisible(!view->isTrafficLayerVisible());
    });
    actTraffic->setCheckable(true);
    actTraffic->setChecked(view->isTrafficLayerVisible());
    actTraffic->setToolTip("Bật/Tắt màu sắc thể hiện mật độ giao thông trên tuyến đường A → B");
    connect(view, &MapView::trafficLayerVisibilityChanged, actTraffic, [actTraffic](bool v) {
      actTraffic->setChecked(v);
    });

    tb->addSeparator();

    tb->addAction("⌖ Về giữa", view, &MapView::resetView);
    tb->addAction("⛶ Toàn cảnh", view, &MapView::fitView);

    tb->addSeparator();

    tb->addAction("▶ Mô phỏng GPS", view, &MapView::playNavigationSimulation);
    tb->addAction("📋 Copy lộ trình", this, &MainWindow::copyRoute);
    tb->addAction("📷 Lưu ảnh", this, &MainWindow::savePng);
    tb->addAction("📍 Danh mục địa điểm (81)", this, &MainWindow::showNodeTable);

    tb->addSeparator();

    QCheckBox *simBox = new QCheckBox("Mô phỏng GT động");
    tb->addWidget(simBox);

    simTimeLabel = new QLabel("🟢 08:00 – Cao điểm sáng");
    simTimeLabel->setStyleSheet(
        "color: #1A73E8; font-weight: bold; padding: 4px 10px; font-size: 13px; "
        "background: #E8F0FE; border-radius: 6px; border: 1.5px solid #1A73E8; margin-left: 4px;");
    tb->addWidget(simTimeLabel);

    connect(view, &MapView::trafficSimTimeChanged, this, [this](int, const QString &desc) {
      if (simTimeLabel) {
        simTimeLabel->setText(QString("🟢 %1").arg(desc));
      }
    });

    connect(simBox, &QCheckBox::toggled, this, [this](bool b) {
      view->setTrafficSimulation(b);
      if (!b && simTimeLabel) {
        simTimeLabel->setText("⚪ Tạm dừng mô phỏng");
      }
    });

    simBox->setChecked(true);
  }

  void buildDirectionsDock() {
    QDockWidget *dock = new QDockWidget("Chỉ đường Google Maps", this);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    QWidget *w = new QWidget;
    w->setStyleSheet("background: #FFFFFF;");
    dockContentWidget = w;
    QVBoxLayout *lay = new QVBoxLayout(w);
    lay->setContentsMargins(14, 14, 14, 14);
    lay->setSpacing(12);

    // Form chọn điểm đi & đến
    QFrame *planCard = new QFrame;
    planCardWidget = planCard;
    planCard->setStyleSheet(R"(
      QFrame {
        background: #FFFFFF;
        border: 1px solid #DADCE0;
        border-radius: 10px;
        padding: 8px;
      }
    )");
    QVBoxLayout *cardLay = new QVBoxLayout(planCard);
    cardLay->setContentsMargins(6, 6, 6, 6);
    cardLay->setSpacing(8);

    QHBoxLayout *rA = new QHBoxLayout;
    QLabel *dotA = new QLabel("🟢");
    dotA->setFixedWidth(22);
    startCombo = new QComboBox;
    startCombo->addItem("— Chọn điểm đi (A) —");
    startCombo->addItems(view->nodeNames());
    connect(startCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    rA->addWidget(dotA);
    rA->addWidget(startCombo, 1);
    cardLay->addLayout(rA);

    QHBoxLayout *rB = new QHBoxLayout;
    QLabel *dotB = new QLabel("🔴");
    dotB->setFixedWidth(22);
    destCombo = new QComboBox;
    destCombo->addItem("— Chọn điểm đến (B) —");
    destCombo->addItems(view->nodeNames());
    connect(destCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onComboChanged);
    rB->addWidget(dotB);
    rB->addWidget(destCombo, 1);
    cardLay->addLayout(rB);

    // Chọn phương tiện
    QHBoxLayout *modeLay = new QHBoxLayout;
    modeLay->setSpacing(6);
    QPushButton *btnCar = new QPushButton("🚗 Ô tô");
    QPushButton *btnBike = new QPushButton("🛵 Xe máy");
    QPushButton *btnWalk = new QPushButton("🚶 Đi bộ");

    QString modeActiveStyle = "QPushButton { background: #E8F0FE; color: #1A73E8; border: 1.5px solid #1A73E8; border-radius: 14px; font-weight: bold; padding: 5px 10px; font-size: 11px; }";
    QString modeNormalStyle = "QPushButton { background: #F1F3F4; color: #5F6368; border: 1px solid transparent; border-radius: 14px; padding: 5px 10px; font-size: 11px; } QPushButton:hover { background: #E8EAED; }";

    btnCar->setStyleSheet(modeActiveStyle);
    btnBike->setStyleSheet(modeNormalStyle);
    btnWalk->setStyleSheet(modeNormalStyle);

    connect(btnCar, &QPushButton::clicked, this, [=]() {
      btnCar->setStyleSheet(modeActiveStyle);
      btnBike->setStyleSheet(modeNormalStyle);
      btnWalk->setStyleSheet(modeNormalStyle);
      view->setTransportMode(MapView::Car);
    });
    connect(btnBike, &QPushButton::clicked, this, [=]() {
      btnBike->setStyleSheet(modeActiveStyle);
      btnCar->setStyleSheet(modeNormalStyle);
      btnWalk->setStyleSheet(modeNormalStyle);
      view->setTransportMode(MapView::Motorbike);
    });
    connect(btnWalk, &QPushButton::clicked, this, [=]() {
      btnWalk->setStyleSheet(modeActiveStyle);
      btnCar->setStyleSheet(modeNormalStyle);
      btnBike->setStyleSheet(modeNormalStyle);
      view->setTransportMode(MapView::Walking);
    });

    modeLay->addWidget(btnCar);
    modeLay->addWidget(btnBike);
    modeLay->addWidget(btnWalk);
    cardLay->addLayout(modeLay);

    lay->addWidget(planCard);

    // Thẻ tổng quan ETA
    QFrame *etaCard = new QFrame;
    etaCardWidget = etaCard;
    etaCard->setStyleSheet(R"(
      QFrame {
        background: #FFFFFF;
        border: 1px solid #DADCE0;
        border-radius: 10px;
        padding: 10px;
      }
    )");
    QVBoxLayout *eLay = new QVBoxLayout(etaCard);
    eLay->setContentsMargins(8, 8, 8, 8);
    eLay->setSpacing(4);

    etaLabel = new QLabel("-- phút");
    etaLabel->setStyleSheet("color: #188038; font-size: 28px; font-weight: bold;");
    eLay->addWidget(etaLabel);

    distEtaLabel = new QLabel("Chưa có tuyến đường");
    distEtaLabel->setStyleSheet("color: #5F6368; font-size: 13px;");
    eLay->addWidget(distEtaLabel);

    trafficChip = new QLabel("● Tuyến nhanh nhất");
    trafficChip->setStyleSheet("background: #E6F4EA; color: #137333; border-radius: 10px; padding: 3px 8px; font-weight: 500; font-size: 11px;");
    eLay->addWidget(trafficChip);

    dualRouteWidget = new QWidget;
    QHBoxLayout *drLay = new QHBoxLayout(dualRouteWidget);
    drLay->setContentsMargins(0, 6, 0, 0);
    drLay->setSpacing(6);
    timeRouteBtn = new QPushButton("⏱ Nhanh nhất");
    distRouteBtn = new QPushButton("📏 Ngắn nhất");
    connect(timeRouteBtn, &QPushButton::clicked, this, [this]() {
      view->setRouteMode(MapView::TimeRoute);
    });
    connect(distRouteBtn, &QPushButton::clicked, this, [this]() {
      view->setRouteMode(MapView::DistRoute);
    });
    drLay->addWidget(timeRouteBtn);
    drLay->addWidget(distRouteBtn);
    dualRouteWidget->setVisible(false);
    eLay->addWidget(dualRouteWidget);

    lay->addWidget(etaCard);

    QLabel *stepsTitle = new QLabel("Chi tiết các chặng di chuyển:");
    stepsTitleLabel = stepsTitle;
    stepsTitle->setStyleSheet("font-weight: bold; color: #202124; font-size: 13px;");
    lay->addWidget(stepsTitle);

    stepList = new QListWidget;
    stepList->setWordWrap(true);
    lay->addWidget(stepList, 1);

    dock->setWidget(w);
    dock->setMinimumWidth(360);
    addDockWidget(Qt::RightDockWidgetArea, dock);
  }

  void buildBottomToolbar() {
    QToolBar *tb = addToolBar("Tùy chọn");
    tb->setMovable(false);

    QCheckBox *unitBox = new QCheckBox("Dùng mét");
    unitBox->setChecked(true);
    connect(unitBox, &QCheckBox::toggled, this, [this](bool b) {
      useMetersFlag = b;
      view->setUseMeters(b);
      onRouteUpdated();
    });
    tb->addWidget(unitBox);

    tb->addSeparator();

    QCheckBox *darkBox = new QCheckBox("Chế độ đêm (Dark)");
    connect(darkBox, &QCheckBox::toggled, this, [this](bool b) {
      view->setDarkMode(b);
      applyAppTheme(b);
    });
    tb->addWidget(darkBox);

    QCheckBox *trafficBox = new QCheckBox("Mật độ GT (A → B)");
    trafficBox->setChecked(view->isTrafficLayerVisible());
    trafficBox->setToolTip("Bật/Tắt màu sắc mật độ giao thông trên tuyến đường A → B");
    connect(trafficBox, &QCheckBox::toggled, view, &MapView::setTrafficLayerVisible);
    connect(view, &MapView::trafficLayerVisibilityChanged, trafficBox, [trafficBox](bool v) {
      trafficBox->setChecked(v);
    });
    tb->addWidget(trafficBox);

    QCheckBox *gridBox = new QCheckBox("Lưới tọa độ");
    connect(gridBox, &QCheckBox::toggled, view, &MapView::setGridVisible);
    tb->addWidget(gridBox);

    tb->addSeparator();

    QCheckBox *banBox = new QCheckBox("Tránh Cầu Rồng");
    connect(banBox, &QCheckBox::toggled, view, &MapView::setBanLongBridge);
    tb->addWidget(banBox);

    QCheckBox *conBox = new QCheckBox("Tránh điểm kẹt xe");
    connect(conBox, &QCheckBox::toggled, view, &MapView::setAvoidCongested);
    tb->addWidget(conBox);

    QCheckBox *altBox = new QCheckBox("So sánh lộ trình");
    connect(altBox, &QCheckBox::toggled, view, &MapView::setShowAlternatives);
    tb->addWidget(altBox);

    tb->addSeparator();

    coordLabel = new QLabel("(—, —)");
    coordLabel->setMinimumWidth(85);
    tb->addWidget(new QLabel(" Tọa độ: "));
    tb->addWidget(coordLabel);

    statsLabel = new QLabel("Giao lộ: 0 | Tuyến đường: 0");
    statsLabel->setMinimumWidth(150);
    tb->addWidget(statsLabel);
  }

protected:
  void showEvent(QShowEvent *e) override {
    QMainWindow::showEvent(e);
    QTimer::singleShot(60, this, [this]() {
      view->resetView();
    });
  }
};

#include "main.moc"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  MainWindow w;
  w.show();
  return app.exec();
}
