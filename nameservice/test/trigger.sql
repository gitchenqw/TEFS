use GFS_test;
delimiter $

DROP TRIGGER IF EXISTS `delblock`$
create trigger delblock after delete on BlockInfo
for each row begin
declare bnum int default 0;
SELECT COUNT(*) INTO bnum FROM BlockInfo WHERE FileID=old.FileID AND Blockindex=old.BlockIndex AND Status=0;
if bnum=0 AND old.Status=0 then
    update FileInfo set FinishedNum=FinishedNum-1 where FileInfo.FileID=old.FileID;
end if;
end;
$

DROP TRIGGER IF EXISTS `cpyblock`$
CREATE TRIGGER cpyblock BEFORE INSERT ON BlockInfo
FOR EACH ROW BEGIN
declare bnum int default 0;
SELECT COUNT(*) INTO bnum FROM BlockInfo WHERE FileID=new.FileID AND Blockindex=new.BlockIndex AND Status=0;
if bnum=0 AND new.Status=0 then
    update FileInfo set FinishedNum=FinishedNum+1 where FileInfo.FileID=new.FileID;
end if;
end;
$

DROP TRIGGER IF EXISTS `updblock`$
CREATE TRIGGER updblock BEFORE UPDATE ON BlockInfo
FOR EACH ROW BEGIN
declare bnum int default 0;
SELECT COUNT(*) INTO bnum FROM BlockInfo WHERE FileID=old.FileID AND Blockindex=old.BlockIndex AND Status=0;
if bnum=0 AND new.Status=0 then
    update FileInfo set FinishedNum=FinishedNum+1 where FileInfo.FileID=old.FileID;
elseif bnum=1 AND new.Status in(1,2) then
    update FileInfo set FinishedNum=FinishedNum-1 where FileInfo.FileID=old.FileID;
end if;
end;
$

delimiter ;