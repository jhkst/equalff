# Maintainer: jhkst <jhkst@centrum.cz>

pkgname=('equalff' 'libequalff' 'libequalff-dev')
_pkgbasename=equalff # Common name for source directory and tarball
pkgver=2.0.0
pkgrel=1
pkgdesc="High-speed duplicate file finder and library"
arch=('x86_64') # Add other architectures if you test and build for them e.g. 'i686' 'aarch64'
url="https://github.com/jhkst/equalff"
license=('GPL3') # SPDX identifier for GPLv3

makedepends=('gcc' 'make')

# Source URL: ensure tag v${pkgver} exists (e.g., v2.0.0)
# The part after '::' renames the downloaded file
source=("${_pkgbasename}-${pkgver}.tar.gz::https://github.com/jhkst/equalff/archive/refs/tags/v${pkgver}.tar.gz")
sha256sums=('SKIP') # IMPORTANT: Replace SKIP with the actual sha256sum of the tarball

prepare() {
  cd "${_pkgbasename}-${pkgver}"
  # If you have patches to apply, do it here. Example:
  # patch -p1 -i "${srcdir}/my-fix.patch"
}

build() {
  cd "${_pkgbasename}-${pkgver}"
  # Arch Linux uses /usr for PREFIX by default, and LIBDIR is typically /usr/lib.
  # Your Makefile uses $(PREFIX)/lib for library installation.
  make PREFIX=/usr
}

package_equalff() {
  pkgdesc="High-speed duplicate file finder (CLI tool)"
  depends=("libequalff=${pkgver}-${pkgrel}") # Runtime dependency on the library package

  cd "${_pkgbasename}-${pkgver}"
  make DESTDIR="${pkgdir}" PREFIX=/usr install

  # Clean up files not belonging to this package
  rm -rf "${pkgdir}/usr/lib"
  rm -rf "${pkgdir}/usr/include"
}

package_libequalff() {
  pkgdesc="Core library for equalff duplicate file finder"
  # No explicit depends here, system handles shared lib dependencies

  cd "${_pkgbasename}-${pkgver}"
  make DESTDIR="${pkgdir}" PREFIX=/usr install

  # Clean up files not belonging to this package
  rm -rf "${pkgdir}/usr/bin"
  rm -rf "${pkgdir}/usr/share/man"
  rm -rf "${pkgdir}/usr/include"
}

package_libequalff-dev() {
  pkgdesc="Development files for libequalff (headers)"
  depends=("libequalff=${pkgver}-${pkgrel}")

  cd "${_pkgbasename}-${pkgver}"
  make DESTDIR="${pkgdir}" PREFIX=/usr install

  # Clean up files not belonging to this package
  # Keep only /usr/include/
  rm -rf "${pkgdir}/usr/bin"
  rm -rf "${pkgdir}/usr/share/man"
  rm -rf "${pkgdir}/usr/lib"
} 