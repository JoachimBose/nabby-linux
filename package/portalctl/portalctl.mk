################################################################################
#
# spookystats
#
################################################################################

PORTALCTL_VERSION = 1.0

PORTALCTL_SITE = /home/vagrant/nabby-linux/package/portalctl
PORTALCTL_SITE_METHOD = local
PORTALCTL_INSTALL_TARGET = YES

$(eval $(cmake-package))
