
use GFS_test;
delimiter $

drop procedure if exists ds_login$
create procedure ds_login(did int,dsize bigint,remain bigint)
begin
    declare num int default 0;
    SELECT COUNT(*) into num FROM DataServer WHERE DSID=did;
    if num=0 then 
        INSERT INTO DataServer(DSID, DiskSize, RemainSize, Status, updatetime) VALUES(did, dsize, remain, 0, now());
    else
        UPDATE DataServer SET DiskSize=dsize, RemainSize=remain, updatetime=now(),Status=0 where DSID=did;
    end if;
end
$

drop procedure if exists upload_file$
create procedure upload_file(fhash Varchar(32), fsize bigint, bnum int, bindex int)
begin
    declare num int default 0;
    declare b_num int default 0;
    declare f_num int default 0;
    declare ret int default 0;
    declare fid bigint default 0;
    declare st  int default 0;

    SELECT COUNT(*), FileID,Status into num, fid,st FROM FileInfo WHERE Hash = fhash;
    if num=0 then 
        INSERT INTO FileInfo(Hash, FileSize, BlockNum, FinishedNum, Status,UploadTime) VALUES(fhash, fsize, bnum, 0, 0, now());
        SELECT FileID INTO fid FROM FileInfo WHERE Hash=fhash;
        SELECT 20 INTO ret;
    else
        if st=1 then
            UPDATE BlockInfo SET Status=0 WHERE FileID=fid AND Status=1;
            UPDATE FileInfo SET Status=0 WHERE FileID=fid;
        end if;

        SELECT BlockNum,FinishedNum into b_num,f_num FROM FileInfo WHERE FileID=fid;
        if b_num=f_num then
            SELECT 13 INTO ret;
        elseif bindex=-1 then
            if f_num>0 then
                SELECT 12 INTO ret;
            else
                SELECT 21 INTO ret;
            end if;
        else
            SELECT COUNT(*) into num FROM BlockInfo WHERE FileID=fid AND Blockindex=bindex AND Status=0;
            if num=0 then
                SELECT 21 INTO ret;
            else
                SELECT 12 INTO ret;
            end if;
        end if;
    end if;

    SELECT ret,fid;
end
$

drop procedure if exists download_file$
create procedure download_file(fhash varchar(32), bindex int)
begin
    declare num int default 0;
    declare out_fid int default 0;
    SELECT COUNT(*),FileID into num,out_fid FROM FileInfo WHERE Hash=fhash AND Status=0;
    if num>0 then
        SELECT out_fid;
        if bindex<0 then
            SELECT Blockindex,DSID FROM BlockInfo WHERE FileID=out_fid AND Status=0 ORDER BY Blockindex;
        else
            SELECT Blockindex,DSID FROM BlockInfo WHERE FileID=out_fid AND Status=0 AND Blockindex=bindex;
        end if;
    else
        SELECT 0 INTO out_fid;
        SELECT out_fid;
    end if;
end
$

drop procedure if exists delete_file$
create procedure delete_file(fhash varchar(32))
begin
    declare fid int default 0;
    declare fnum int default 0;
    declare bnum int default 0;
    SELECT FileID,FinishedNum into fid,fnum FROM FileInfo WHERE Hash=fhash;
    if fid>0 AND fnum>0 then
        UPDATE FileInfo SET Status=1 where FileID=fid AND Status=0;
        UPDATE BlockInfo SET Status=1 where FileID=fid AND Status=0;
    elseif fid>0 AND fnum=0 then
        SELECT COUNT(*) INTO bnum FROM BlockInfo WHERE FileID=fid;
        if bnum=0 then
            DELETE FROM FileInfo WHERE FileID=fid;
        end if;
    end if;
end
$

drop procedure if exists delete_block$
create procedure delete_block(fid bigint, bindex int, ds_id int)
begin
    declare bnum int default 0;
    DELETE FROM BlockInfo WHERE FileID=fid AND Blockindex=bindex AND DSID=ds_id AND Status=2;
    SELECT COUNT(*) INTO bnum FROM BlockInfo WHERE FileID=fid;
    if bnum=0 then
        DELETE FROM FileInfo WHERE FileID=fid;
    end if;
end
$

drop procedure if exists copy_block$
create procedure copy_block(fid bigint, bindex int, ds_id int)
begin
    declare bnum int default 0;
    declare fnum int default 0;
    declare bid  int default 0;
    SELECT COUNT(*) INTO fnum FROM FileInfo WHERE FileID=fid;
    if fnum>0 then
        SELECT COUNT(*),ID INTO bnum,bid FROM BlockInfo WHERE FileID=fid AND Blockindex=bindex AND DSID=ds_id;
        if bnum=0 then
            INSERT INTO BlockInfo(Blockindex,DSID,FileID,Status) VALUES(bindex,ds_id,fid,0);
        else
            UPDATE BlockInfo SET Status=0 WHERE ID=bid;
        end if;
    end if;
end
$

drop procedure if exists get_del_block$
create procedure get_del_block(opt int, copies int, idx int)
begin
    set @page_num = 50;
    if opt=0 then              #状态为删除标记的块
        PREPARE sql1 FROM 'SELECT FileID,Blockindex,DSID FROM BlockInfo WHERE Status IN(1,2) limit ?,?';
        SET @a = idx*@page_num;
        SET @b = @page_num;
        EXECUTE sql1 USING @a,@b;
        DEALLOCATE PREPARE sql1;
    elseif opt=1 then          #备份数大于copies的块
        PREPARE sql2 FROM 'SELECT TBLOCK.FileID,TBLOCK.Blockindex,BlockInfo.DSID FROM (SELECT FileID,Blockindex,COUNT(*) AS num FROM BlockInfo GROUP BY FileID,Blockindex) AS TBLOCK,BlockInfo WHERE TBLOCK.FileID=BlockInfo.FileID AND TBLOCK.Blockindex=BlockInfo.Blockindex AND num>? limit ?,?';
        SET @a=idx*@page_num;
        SET @b=@page_num;
        set @num=copies;
        EXECUTE sql2 USING @num,@a,@b;
        DEALLOCATE PREPARE sql2;
    end if;
end
$

drop procedure if exists get_cpy_block$
create procedure get_cpy_block(copies int, idx int)
begin
    set @page_num = 50;
    PREPARE sql1 FROM 'SELECT TBLOCK.FileID,TBLOCK.Blockindex,BlockInfo.DSID FROM (SELECT FileID,Blockindex,COUNT(*) AS num FROM BlockInfo GROUP BY FileID,Blockindex) AS TBLOCK,BlockInfo WHERE TBLOCK.FileID=BlockInfo.FileID AND TBLOCK.Blockindex=BlockInfo.Blockindex AND num<? limit ?,?';
    SET @a=idx*@page_num;
    SET @b=@page_num;
    set @num=copies;
    EXECUTE sql1 USING @num,@a,@b;
    DEALLOCATE PREPARE sql1;
end
$

drop procedure if exists get_fsize$
create procedure get_fsize(fhash varchar(32))
begin
    declare fsize bigint default 0;
    SELECT FileSize into fsize FROM FileInfo WHERE Hash=fhash AND Status=0;
    SELECT fsize;
end
$

drop procedure if exists set_del_ing$
create procedure set_del_ing(fid bigint, bindex int, ds_id int)
begin
    UPDATE BlockInfo SET Status=2 WHERE FileID=fid AND Blockindex=bindex AND DSID=ds_id;
end
$

delimiter ;
