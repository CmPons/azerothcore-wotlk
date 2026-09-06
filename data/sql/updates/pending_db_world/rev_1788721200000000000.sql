-- Razorgore's phase-one Explosion (20038), effect 1 (mask 2), must only despawn eggs.
-- The spell ignores line of sight and otherwise also despawns suppression devices,
-- doors and other unrelated objects in nearby rooms. Keep effect 0's player filter.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceGroup` = 2 AND `SourceEntry` = 20038 AND `SourceId` = 0;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`) VALUES
(13, 2, 20038, 0, 0, 31, 0, 5, 177807, 0, 0, 0, 0, '', 'Razorgore - Explosion despawns only Black Dragon Eggs');
