################################################################################
#
# spookystats
#
################################################################################

SPOOKYSTATS_VERSION = 1.0

SPOOKYSTATS_SITE = /home/vagrant/nabby-linux/package/spookystats
SPOOKYSTATS_SITE_METHOD = local
SPOOKYSTATS_INSTALL_TARGET = YES

$(eval $(cmake-package))
