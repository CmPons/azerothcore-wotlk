-- Conditional old-raid reset deadlines. No existing binds, encounters or timers are modified.
CREATE TABLE IF NOT EXISTS `instance_progression_reset` (
  `instanceId` int unsigned NOT NULL,
  `stage` tinyint unsigned NOT NULL COMMENT '1 fresh, 2 progression, 3 cleared',
  `resetTime` bigint unsigned NOT NULL,
  `extendedResetTime` bigint unsigned NOT NULL,
  PRIMARY KEY (`instanceId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
