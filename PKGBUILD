# This is an example PKGBUILD file. Use this as a start to creating your own,
# and remove these comments. For more information, see 'man PKGBUILD'.
# NOTE: Please fill out the license field for your package! If it is unknown,
# then please put 'unknown'.

# Maintainer: Your Name <youremail@domain.com>
pkgname=dwmstatus
pkgver=1.0
pkgrel=1
pkgdesc='A barebone status monitor'
arch=('x86_64')
url='git://git.suckless.org/dwmstatus'
license=('MIT')
depends=('libx11')
source=('git://git.suckless.org/dwmstatus')
md5sums=('SKIP')

build() {
	cd "$pkgname"
	make
    make PREFIX=/usr install
}

package() {
	cd "$pkgname"
	make DESTDIR="$pkgdir/" install
}

md5sums=('SKIP')
