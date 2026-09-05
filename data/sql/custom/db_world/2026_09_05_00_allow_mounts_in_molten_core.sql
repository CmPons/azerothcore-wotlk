-- Allow old-school raid groups to use mounts inside Molten Core.
UPDATE `instance_template` SET `allowMount` = 1 WHERE `map` = 409;
