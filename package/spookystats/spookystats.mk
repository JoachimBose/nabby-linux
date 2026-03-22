################################################################################
#
# spookystats
#
################################################################################

SPOOKY_STATS_VERSION = 1.0
SPOOKY_STATS_SITE = $(BR2_EXTERNAL_NABBY_LINUX_PATH)/package/spookystats
SPOOKY_STATS_SITE_METHOD = local
SPOOKY_STATS_INSTALL_TARGET = YES

$(eval $(cmake-package))