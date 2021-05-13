import { Component, OnInit, OnDestroy, ViewChild } from '@angular/core';
import { FormBuilder, Validators } from '@angular/forms';

import { ConfigService } from 'src/app/config.service';
import { HardDiskManagementInterface } from './HardDiskManagementInterface';
import { QuotaInterface } from './QuotaInterface';
import { isNumberJudge } from 'src/app/shared/validators/benumber.directive';
import { IeCssService } from 'src/app/shared/func-service/ie-css.service';
import { ResponseErrorService } from 'src/app/shared/func-service/response-error.service';
import { EmployeeService, BoolEmployee, EmployeeItem } from 'src/app/shared/func-service/employee.service';
import { LockService } from 'src/app/shared/func-service/lock-service.service';
import { TipsService } from 'src/app/tips/tips.service';
import { PublicFuncService } from 'src/app/shared/func-service/public-func.service';
import Logger from 'src/app/logger';

@Component({
  selector: 'app-hard-disk-management',
  templateUrl: './hard-disk-management.component.html',
  styleUrls: ['./hard-disk-management.component.scss']
})
export class HardDiskManagementComponent implements OnInit, OnDestroy {

  hddList: HardDiskManagementInterface[] = [];

  constructor(
    private cfgService: ConfigService,
    private fb: FormBuilder,
    private ieCss: IeCssService,
    private resError: ResponseErrorService,
    private ES: EmployeeService,
    private tips: TipsService,
    private pfs: PublicFuncService,
  ) { }

  private logger: Logger = new Logger('hard-disk-management');

  isChrome: boolean = false;
  isFormat: boolean = false;
  lock = new LockService(this.tips);
  employee = new BoolEmployee();
  epBank: EmployeeItem[] = [
    {
      name: 'init',
      task: ['list', 'path']
    }
  ];

  selectPath: string = '';
  formatEmployeeName: string = 'hard-disk-format';
  avalidDisk: Array<number> = [];

  HardDiskManagementInterfaceForm = this.fb.group({
    selected: [false],
    id: [''],
    iFormatStatus: [''],
    iFormatProg: [''],
    iFreeSize: [''],
    iMediaSize: [''],
    sStatus: [''],
    iTotalSize: [''],
    sAttributes: [''],
    sDev: [''],
    sMountPath: [''],
    sName: [''],
    sType: ['']
  });

  QuotaInterfaceForm = this.fb.group({
    id: [''],
    iFreePictureQuota: [''],
    iFreeVideoQuota: [''],
    iTotalPictureVolume: [''],
    iTotalVideoVolume: [''],
    iPictureQuotaRatio: ['', [Validators.required, isNumberJudge]],
    iVideoQuotaRatio: ['', [Validators.required, isNumberJudge]]
  });

  selectForm = this.fb.group({
    selectAll: [false],
  });

  ngOnInit(): void {
    this.isChrome = this.ieCss.getChromeBool();
    this.updateHddList();
    this.employee.hire(this.epBank[0]);
    this.cfgService.getStoragePath().subscribe(
      res => {
        this.resError.analyseRes(res);
        this.selectPath = res['sMountPath'];
        this.employee.doTask(this.epBank[0].name, this.epBank[0].task[0], true);
      },
      err => {
        this.logger.error(err, 'ngOnInit');
        this.employee.doTask(this.epBank[0].name, this.epBank[0].task[0], false);
      }
    );
    this.employee.observeTask(this.epBank[0].name, 10000)
      .then(() => {
        if (this.employee.getWorkResult(this.epBank[0].name)) {
          let idNum = 3;
          for (const item of this.hddList) {
            if (item.sMountPath === this.selectPath) {
              idNum = item.id;
              break;
            }
          }
          this.updateQuota(idNum);
          this.QuotaInterfaceForm.get('iFreePictureQuota').disable();
          this.QuotaInterfaceForm.get('iFreeVideoQuota').disable();
          this.QuotaInterfaceForm.get('iTotalPictureVolume').disable();
          this.QuotaInterfaceForm.get('iTotalVideoVolume').disable();
          this.employee.dismissOne(this.epBank[0].name);
        } else {
          this.tips.showInitFail();
          this.logger.error('ngOnInit:observeTask:no-name');
          this.employee.dismissOne(this.epBank[0].name);
        }
      })
      .catch((error) => {
        this.logger.error(error, 'ngOnInit:observeTask:catch');
        this.tips.showInitFail();
        this.employee.dismissOne(this.epBank[0].name);
      });
  }

  ngOnDestroy() {
    this.ES.dismissOne(this.formatEmployeeName);
  }

  sizeCompare(size1: number, size2: number) {
    if (size1 === size2) {
      return true;
    }
    const bigSize = Math.max(size1, size2);
    const smallSize = Math.min(size1, size2);
    const deltaSize = bigSize - smallSize;
    return deltaSize < 0.001;
  }

  updateHddList(): void {
    if (this.lock.checkLock('updateHddList')) {
      return;
    }
    this.lock.lock('updateHddList');
    this.cfgService.getHardDiskManagementInterface().subscribe(
      (res: HardDiskManagementInterface[]) => {
        this.resError.analyseRes(res);
        const Len = res.length;
        this.hddList = [];
        for (let i = 0; i < Len; i++) {
          this.hddList.push(null);
        }
        for (let i = 0; i < Len; i++) {
          res[i].iTotalSize = Number(res[i].iTotalSize.toFixed(3));
          res[i].iFreeSize = Number(res[i].iFreeSize.toFixed(3));
          res[i].selected = false;
          this.hddList[res[i]['id'] - 1] = res[i];
        }
        // add for options
        this.updateAvalidDisk(res);
        this.HardDiskManagementInterfaceForm.patchValue(res);
        this.employee.doTask(this.epBank[0].name, this.epBank[0].task[1], true, null);
        this.lock.unlock('updateHddList');
      },
      err => {
        this.logger.error(err, 'updateHddList');
        this.employee.doTask(this.epBank[0].name, this.epBank[0].task[1], false, null);
        this.lock.unlock('updateHddList');
      }
    );
  }

  updateAvalidDisk(rawRes: HardDiskManagementInterface[]) {
    this.avalidDisk = [];
    for (const item of rawRes) {
      if (item.sStatus === 'mounted') {
        this.avalidDisk.push(item.id);
      }
    }
    for (const item of this.avalidDisk) {
      if (!this.QuotaInterfaceForm.value.id || this.QuotaInterfaceForm.value.id === item) {
        return;
      }
    }
    if (this.avalidDisk.length > 0) {
      this.QuotaInterfaceForm.get('id').setValue(this.avalidDisk[0]);
    } else {
      this.tips.setRbTip('noAvalidDisk');
      this.logger.error('updateAvalidDisk:noAvalidDisk');
    }
  }

  updateQuota(quotaId: number): void {
    if (this.lock.checkLock('updateQuota')) {
      return;
    }
    this.lock.lock('updateQuota');
    this.cfgService.getQuotaInterface(quotaId).subscribe(
      (res: QuotaInterface) => {
        this.resError.analyseRes(res);
        res.iFreePictureQuota = Number(res.iFreePictureQuota.toFixed(3));
        res.iFreeVideoQuota = Number(res.iFreeVideoQuota.toFixed(3));
        res.iTotalPictureVolume = Number(res.iTotalPictureVolume.toFixed(3));
        res.iTotalVideoVolume = Number(res.iTotalVideoVolume.toFixed(3));
        this.QuotaInterfaceForm.patchValue(res);
        this.lock.unlock('updateQuota');
      },
      err => {
        this.tips.setRbTip('inquiryQuotaFail');
        this.logger.error('updateQuota:inquiryQuotaFail');
        this.lock.unlock('updateQuota');
      }
    );
  }

  onSubmit() {
    this.pfs.formatInt(this.QuotaInterfaceForm.value);
    this.cfgService.setQuotaInterface(this.QuotaInterfaceForm.value.id, this.QuotaInterfaceForm.value)
      .subscribe(res => {
      this.resError.analyseRes(res);
      this.tips.showSaveSuccess();
    },
    err => {
      this.logger.error(err, 'onSubmit:setQuotaInterface:');
      this.tips.showSaveFail();
    });
  }

  format() {
    if (!this.ES.hireOne(this.formatEmployeeName)) {
      this.tips.setRbTip('formatingNotClick');
      this.logger.error('format:formatingNotClick:');
      return;
    }
    const Len = this.hddList.length;
    let formatCnt = 0;
    for (let i = 0; i < Len; i++) {
      if (this.hddList[i].selected === true) {
        formatCnt += 1;
        this.ES.pushTaskList(this.formatEmployeeName, this.hddList[i].id.toString());
        this.cfgService.formatHardDisk(i + 1).subscribe(res => {
          },
          err => {
            this.logger.error(err, 'format');
          });
      }
    }
    if (formatCnt <= 0) {
      this.tips.setRbTip('noDiskSelect');
      this.logger.error('format:noDiskSelect:');
      this.ES.dismissOne(this.formatEmployeeName);
      return;
    }
    this.logger.debug('format number: ' + formatCnt);
    this.inquireFormatStatus();
    this.ES.observeTask(this.formatEmployeeName)
      .then(
        () => {
          this.tips.setRbTip('completeFormating');
          this.ES.dismissOne(this.formatEmployeeName);
        }
      )
      .catch(
        (error) => {
          this.tips.setRbTip('getFormatStatusFailFresh');
          this.logger.error(error, 'format:observeTask:catch:');
          this.ES.dismissOne(this.formatEmployeeName);
        }
      );
  }

  inquireFormatStatus() {
    const that = this;
    setTimeout(() => {
      if (!that.ES.isTaskDone(that.formatEmployeeName)) {
        that.cfgService.getHardDiskManagementInterface().subscribe(
          (res: HardDiskManagementInterface[]) => {
            const msg = that.resError.isErrorCode(res);
            if (msg) {
              that.logger.error(msg, 'inquireFormatStatus:getHardDiskManagementInterface:resError:');
            } else {
              const taskList = that.ES.getTaskList(that.formatEmployeeName);
              this.logger.debug(taskList, 'taskList:');
              for (const item of res) {
                if (this.pfs.isInArrayString(taskList, item.id.toString())) {
                  if (item.sName === 'Emmc' && item.sStatus === 'mounted') {
                    that.hddList[item.id - 1].iFormatProg = 100;
                    that.ES.doTask(that.formatEmployeeName, item.id.toString());
                    that.updateHddList();
                  } else if (item.sName === 'Emmc' && item.sStatus !== 'mounted') {
                    if (that.hddList[item.id - 1].iFormatProg < 90) {
                      that.hddList[item.id - 1].iFormatProg += Math.ceil(Math.random() * 10);
                    }
                  } else {
                    that.hddList[item.id - 1].iFormatStatus = item.iFormatStatus;
                    that.hddList[item.id - 1].iFormatProg = item.iFormatProg;
                    that.hddList[item.id - 1].sStatus = item.sStatus;
                    that.hddList[item.id - 1].iFreeSize = item.iFreeSize;
                    if (that.sizeCompare(that.hddList[item.id - 1].iTotalSize, that.hddList[item.id - 1].iFreeSize)) {
                      that.ES.doTask(that.formatEmployeeName, item.id - 1);
                      that.updateHddList();
                    }
                  }
                }
              }
            }
            that.inquireFormatStatus();
          },
          err => {
            that.inquireFormatStatus();
          }
        );
      }
    }, 1000);
  }

  checkAll(change: any) {
    this.lock.lock('checkAll');
    const Len = this.hddList.length;
    let changeCount = 0;
    for (let i = 0; i < Len; i++) {
      // if (this.hddList[i].sStatus !== 'unmounted' && this.hddList[i].sName !== 'Emmc') {
      if (this.hddList[i].sStatus !== 'unmounted') {
        changeCount += 1;
        this.hddList[i].selected = this.selectForm.value.selectAll;
      }
    }
    if (changeCount === 0) {
      this.selectForm.get('selectAll').setValue(!this.selectForm.value.selectAll);
      this.tips.setRbTip('noDiskCanBeSelected');
    }
    this.lock.unlock('checkAll');
  }

  onSelectedHardDisk(info: any) {
    if (info.sStatus !== 'unmounted') {
      // if (info.sName !== 'Emmc') {
      this.hddList[info.id - 1].selected = !this.hddList[info.id - 1].selected;
      // }
      this.selectPath = info.sMountPath;
      this.updateQuota(info.id);
    } else {
      this.tips.setRbTip('diskIsUnmounted');
    }
  }

  onSwitchDisk(id: number | string) {
    if (this.lock.checkLock('onSwitchDisk')) {
      return;
    }
    this.lock.lock('onSwitchDisk');
    id = Number(id);
    if (this.isHardDiskSelected(this.hddList[id - 1])) {
      this.lock.unlock('onSwitchDisk');
      return;
    }
    this.selectPath = this.hddList[id - 1].sMountPath;
    this.updateQuota(id);
    this.lock.unlock('onSwitchDisk');
  }

  isHardDiskSelected(info: any) {
    if (info['sMountPath'] === this.selectPath) {
      return true;
    } else {
      return false;
    }
  }
}
