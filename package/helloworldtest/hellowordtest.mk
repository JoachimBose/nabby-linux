################################################################################
#
# helloworldtest
#
################################################################################

HELLOWORLDTEST_VERSION = 1.0

HELLOWORLDTEST_SITE = $(BR2_EXTERNAL_NABBY_LINUX_PATH)/package/helloworldtest
HELLOWORLDTEST_SITE_METHOD = local
HELLOWORLDTEST_INSTALL_TARGET = YES

$(eval $(cmake-package))
