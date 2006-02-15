#include "TrackingTools/GeomPropagators/interface/HelixBarrelCylinderCrossing.h"
#include "Geometry/Surface/interface/Cylinder.h"
#include "TrackingTools/GeomPropagators/src/RealQuadEquation.h"
#include <iostream>
#include <cmath>

template <typename T> 
inline T sqr(const T& t) {return t*t;}

HelixBarrelCylinderCrossing::
HelixBarrelCylinderCrossing( const GlobalPoint& startingPos,
			     const GlobalVector& startingDir,
			     double rho, PropagationDirection propDir, 
			     const Cylinder& cyl)
{
  // assumes the cylinder is centered at 0,0
  double R = cyl.radius();
  double R2cyl = R*R;
  double pt   = startingDir.perp();
  Point center( startingPos.x()-startingDir.y()/(pt*rho),
		startingPos.y()+startingDir.x()/(pt*rho));
  double p2 = startingPos.perp2();
  bool solveForX;
  double B, C, E, F;
  if (fabs(center.x()) > fabs(center.y())) {
    solveForX = false;
    E = (R2cyl - p2) / (2.*center.x());
    F = center.y()/center.x();
    B = 2.*( startingPos.y() - F*startingPos.x() - E*F);
    C = 2.*E*startingPos.x() + E*E + p2 - R2cyl;
  }
  else {
    solveForX = true;
    E = (R2cyl - p2) / (2.*center.y());
    F = center.x()/center.y();
    B = 2.*( startingPos.x() - F*startingPos.y() - E*F);
    C = 2.*E*startingPos.y() + E*E + p2 - R2cyl;
  }

  RealQuadEquation eq( 1+F*F, B, C);
  if (!eq.hasSolution) {
    theSolExists = false;
    return;
  }

  Vector d1, d2;;
  if (solveForX) {
    d1 = Point(eq.first,  E-F*eq.first);
    d2 = Point(eq.second, E-F*eq.second);
  }
  else {
    d1 = Point( E-F*eq.first,  eq.first);
    d2 = Point( E-F*eq.second, eq.second);
  }
  
  chooseSolution(d1, d2, startingPos, startingDir, propDir);
  if (!theSolExists) return;

  double pabs = startingDir.mag();
  double sinTheta = pt / pabs;
  double cosTheta = startingDir.z() / pabs;

  double dMag = theD.mag();
  theS = theActualDir * 2.* asin( dMag*rho/2.) / (rho*sinTheta);
  thePos =  GlobalPoint( startingPos.x() + theD.x(),
			 startingPos.y() + theD.y(),
			 startingPos.z() + theS*cosTheta);

  double tmp = 0.5 * dMag * rho;
  if (theS < 0) tmp = -tmp;
  double sinPhi = 2.*tmp*sqrt(1.-tmp*tmp);
  double cosPhi = 1.-2.*tmp*tmp;
  theDir = DirectionType(startingDir.x()*cosPhi-startingDir.y()*sinPhi,
			 startingDir.x()*sinPhi+startingDir.y()*cosPhi,
			 startingDir.z());
}
void HelixBarrelCylinderCrossing::chooseSolution( const Vector& d1, const Vector& d2,
						  const PositionType& startingPos,
						  const DirectionType& startingDir, 
						  PropagationDirection propDir)
{
  double momProj1 = startingDir.x()*d1.x() + startingDir.y()*d1.y();
  double momProj2 = startingDir.x()*d2.x() + startingDir.y()*d2.y();

  if ( propDir == anyDirection ) {
    theSolExists = true;
    if (d1.mag2()<d2.mag2()) {
      theD = d1;
      theActualDir = (momProj1 > 0) ? 1 : -1;
    }
    else {
      theD = d2;
      theActualDir = (momProj2 > 0) ? 1 : -1;
    }
  }
  else {
    int propSign = propDir==alongMomentum ? 1 : -1;
    if (momProj1*momProj2 < 0) {
      // if different signs return the positive one
      theSolExists = true;
      theD = (momProj1*propSign > 0) ? d1 : d2;
      theActualDir = propSign;
    }
    else if (momProj1*propSign > 0) {
      // if both positive, return the shortest
      theSolExists = true;
      theD = (d1.mag2()<d2.mag2()) ? d1 : d2;
      theActualDir = propSign;
    }
    else theSolExists = false;
  }
}

