drop database if exists GFS;
create database GFS character set utf8;

use GFS;

create table DataServer
(
    `DSID`        int unsigned not null primary key,
    `DiskSize`    bigint unsigned comment '硬盘大小',
    `RemainSize`  bigint comment '剩余硬盘大小',
    `Status`      tinyint not null comment 'DS状态：0正常，1不正常',
    `updatetime`  datetime not null
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

create table BlockInfo
(
    `ID`          int unsigned not null AUTO_INCREMENT primary key,
    `Blockindex`  int not null,
    `DSID`        int unsigned not null,
    `FileID`      bigint not null,
    `Status`      tinyint not null comment '文件块状态：0正常，1删除，2删除中',
    `updatetime`  timestamp not null default current_timestamp
)ENGINE=InnoDB DEFAULT CHARSET=utf8;

create table FileInfo
(
    `FileID`     bigint unsigned not null AUTO_INCREMENT primary key,
    `Hash`       varchar(32) not null comment '文件MD5值',
    `FileSize`   bigint comment '文件大小',
    `BlockNum`   int not null comment '文件块个数',
    `FinishedNum` int not null default 0 comment '已上传完成文件块个数',
    `UploadTime` Datetime comment '文件上传完成时间',
    `Status`     tinyint not null comment '文件状态：0正常，1删除'
)ENGINE=InnoDB DEFAULT CHARSET=utf8 AUTO_INCREMENT=1;



