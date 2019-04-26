# Install script for directory: /home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/Eigen" TYPE FILE FILES
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Cholesky"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/CholmodSupport"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Core"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Dense"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Eigen"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Eigenvalues"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Geometry"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Householder"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/IterativeLinearSolvers"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Jacobi"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/LU"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/MetisSupport"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/OrderingMethods"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/PaStiXSupport"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/PardisoSupport"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/QR"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/QtAlignedMalloc"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/SPQRSupport"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/SVD"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/Sparse"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/SparseCholesky"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/SparseCore"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/SparseLU"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/SparseQR"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/StdDeque"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/StdList"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/StdVector"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/SuperLUSupport"
    "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/UmfPackSupport"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/Eigen" TYPE DIRECTORY FILES "/home/anneriet/Afstuderen/eigen-eigen-323c052e1731/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

