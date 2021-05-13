import { Component, OnInit, AfterViewInit, OnDestroy, ViewChild, ElementRef, Renderer2, HostListener, Input } from '@angular/core';

import { Subject } from 'rxjs';
import Player from './Player';
import Logger from 'src/app/logger';
import videojs from 'video.js';
import 'videojs-flash';

import { ConfigService } from 'src/app/config.service';
import { TipsService } from 'src/app/tips/tips.service';
import { LockService } from 'src/app/shared/func-service/lock-service.service';
import { PublicFuncService } from 'src/app/shared/func-service/public-func.service';
import { ResponseErrorService } from 'src/app/shared/func-service/response-error.service';
import { BoolEmployee } from 'src/app/shared/func-service/employee.service';
import { LayoutService } from 'src/app/shared/func-service/layout.service';
import { IeCssService } from 'src/app/shared/func-service/ie-css.service';
import { DiyHttpService } from 'src/app/shared/func-service/diy-http.service';

@Component({
  selector: 'app-player',
  templateUrl: './player.component.html',
  styleUrls: ['./player.component.scss']
})
export class PlayerComponent implements OnInit, AfterViewInit, OnDestroy {

  @Input() option: any;

  @ViewChild('canvasvideo', { static: true })
  canvasVideo: ElementRef<HTMLCanvasElement>;
  @ViewChild('canvasdraw', { static: true })
  canvasDraw: ElementRef<HTMLCanvasElement>;
  @ViewChild('canvasbox', { static: true })
  canvasBox: ElementRef<HTMLCanvasElement>;
  @ViewChild('video', {static: true})
  videoChild: ElementRef<HTMLVideoElement>;
  @ViewChild('video', {static: false})
  videoPlayer: ElementRef<HTMLVideoElement>;
  @ViewChild('videoEdge', {static: true})
  videoEdgeChild: ElementRef;

  constructor(
    private Re2: Renderer2,
    private el: ElementRef,
    private cfg: ConfigService,
    private tips: TipsService,
    private pfs: PublicFuncService,
    private resCheck: ResponseErrorService,
    private los: LayoutService,
    private ics: IeCssService,
    private dhs: DiyHttpService,
  ) { }

  private saveSignal: Subject<boolean> = new Subject<boolean>();

  lock = new LockService(this.tips);
  private ctOb: any;
  private resolutionOb: any;
  private employee = new BoolEmployee();
  private start_x: number = 0;
  private start_y: number = 0;
  private x: number = 0;
  private y: number = 0;
  private rect: any;
  private ctx: any;
  private aspect: number = 9 / 16;
  private defaultAspect: number = 9 / 16;
  private player: Player;
  tipContent: string;
  private imageMode: boolean = false;
  private playerCtx: any;
  private isIe: boolean = false;

  private isMenuEnabled: boolean = false;
  isPlanEnabled: boolean = false;
  private drawerHeightProportion: number;
  private drawerWidthProportion: number;
  private drawArray: DrawCell[];
  private charLength: number = 16;
  private pointEnabled: boolean = false;
  isDrawing: boolean = false;
  private drawingStatus: string = 'none';
  private pointIndex: any;
  private activeIndex: any = 'none';
  private drawEnabled: any;
  private moveEnabled: boolean = true;
  private normalHeight: number;
  private normalWidth: number;
  private motionMode: boolean = false;
  private motionX: number;
  private motionY: number;
  private motionXCount: number;
  private motionYCount: number;
  private isViewInit: boolean = false;
  size = {
    height: 0,
    width: 0,
  };

  logger = new Logger('playerComponent');
  private ws: any = null;
  video: any = null;
  isRtmp: boolean = false;
  isPlaying: boolean = false;
  isMute: boolean = false;
  private displayUrl: string;
  private videoBigPlayShowStatus: boolean = true;

  isRecording: boolean = false;
  recordTitleBank = {
    true: 'stopRecording',
    false: 'startRecording',
  };

  videoPara =  {
    isReady: false,
    height: 0,
    width: 0
  };

  activePoint = [
    {
      x: 0,
      y: 0,
    },
    {
      x: 0,
      y: 0,
    },
    {
      x: 0,
      y: 0,
    },
    {
      x: 0,
      y: 0,
    },
    {
      x: 0,
      y: 0,
    },
    {
      x: 0,
      y: 0,
    },
    {
      x: 0,
      y: 0,
    },
    {
      x: 0,
      y: 0,
    },
  ];

  epBank = [
    {
      name: 'recording',
      task: ['record', 'plan']
    }
  ];

  expendBtn: any = {
    all: false,
    save: false,
    undraw: false,
  };

  get SaveSignal() {
    return this.saveSignal.asObservable();
  }

  ngOnInit() {
    this.isIe = this.ics.getIEBool();

    if (this.option['imageMode']) {
      this.imageMode = this.option['imageMode'];
    }

    if (this.option.isReshape || this.isIe) {
      this.aspect = 0.75;
      this.resolutionOb = this.los.PlayerResolution.subscribe(
        (resolution: string) => {
          if (resolution) {
            for (let i = 0; i < resolution.length; i++) {
              if (resolution[i] === '*') {
                this.videoPara.height = Number(resolution.slice(i + 1));
                this.videoPara.width = Number(resolution.slice(0, i));
                this.aspect = Number(resolution.slice(i + 1)) / Number(resolution.slice(0, i));
                this.watiViewInit(5000).then(() => {
                  this.resizeCanvas(this.aspect);
                  if (this.drawArray) {
                    this.drawWhenResize();
                  }
                })
                .catch(
                  (error) => {
                    this.logger.log(error, 'ngOnInit:getVideoEncoderInterface:watiViewInit:error');
                  }
                );
                if (this.resolutionOb) {
                  this.resolutionOb.unsubscribe();
                  this.resolutionOb = null;
                }
                break;
              }
            }
          }
        }
      );
    }

    this.expendBtnSwitch();
  }

  ngAfterViewInit() {
    this.isViewInit = true;
    this.canvasDraw.nativeElement.addEventListener('mousedown', e => {
      this.rect = this.canvasDraw.nativeElement.getBoundingClientRect();
      this.start_x = e.clientX - this.rect.left;
      this.start_y = e.clientY - this.rect.top;
      if (this.motionMode  && this.isDrawing) {
        this.drawingStatus = 'motion';
        this.motionX = this.start_x;
        this.motionY = this.start_y;
      } else {
        this.pointIndex = this.clickOnWhichPoint(this.start_x, this.start_y);
        if (this.pointIndex !== 'none' && this.pointEnabled) {
          this.drawingStatus = 'point';
        } else {
          this.activeIndex = this.clickOnWhich(this.start_x, this.start_y);
          if (this.activeIndex !== 'none' && this.moveEnabled) {
            this.setActivePoint(this.activeIndex);
            this.drawingStatus = 'move';
          } else {
            if (this.isDrawing) {
              this.drawEnabled = this.checkDrawEnable();
              if (this.drawEnabled !== 'none') {
                this.drawingStatus = 'draw';
                this.activeIndex = this.drawEnabled;
                this.drawArray[this.drawEnabled].x = this.start_x;
                this.drawArray[this.drawEnabled].y = this.start_y;
              } else {
                this.activeIndex = 'none';
              }
            } else {
              this.activeIndex = 'none';
            }
          }
        }
      }
      this.drawPic();
    });

    this.canvasDraw.nativeElement.addEventListener('mousemove', (e: any) => {
      if (this.drawingStatus !== 'none') {
        this.x = e.clientX - this.rect.left;
        this.y = e.clientY - this.rect.top;
        const deltaX = this.x - this.start_x;
        const deltaY = this.y - this.start_y;
        this.start_x = this.x;
        this.start_y = this.y;
        if (this.drawingStatus === 'draw') {
          this.drawArray[this.drawEnabled].width += deltaX;
          this.drawArray[this.drawEnabled].height += deltaY;
          this.setActivePoint(this.activeIndex);
        } else if (this.drawingStatus === 'point') {
          this.pointMove(this.pointIndex, deltaX, deltaY);
          this.pointSetRect(this.activeIndex);
        } else if (this.drawingStatus === 'move') {
          this.RectMove(this.activeIndex, deltaX, deltaY);
          this.setActivePoint(this.activeIndex);
        } else if (this.drawingStatus === 'motion') {
          this.motionCheck();
        }
        if (e.clientX < this.rect.left + 1
          || e.clientX > this.rect.right - 5
          || e.clientY < this.rect.top + 1
          || e.clientY > this.rect.bottom - 3) {
            this.drawingStatus = 'none';
          }
        this.drawPic();
      }
    });

    this.canvasDraw.nativeElement.addEventListener('mouseup', e => {
      this.drawingStatus = 'none';
    });

    this.resizeCanvas(this.aspect);
    this.ctx = this.canvasDraw.nativeElement.getContext('2d');
  }

  ngOnDestroy() {
    if (this.video) {
      this.video.dispose();
    }
    if (this.ctOb) {
      this.ctOb.unsubscribe();
    }
    if (this.resolutionOb) {
      this.resolutionOb.unsubscribe();
      this.resolutionOb = null;
    }
  }

  watiViewInit = (timeoutms: number) => new Promise(
    (resolve, reject) => {
      const isViewInitDone = () => {
        timeoutms -= 100;
        if (this.isViewInit) {
          resolve();
        } else if (timeoutms <= 0) {
          reject();
        } else {
          setTimeout(isViewInitDone, 100);
        }
      };
      isViewInitDone();
    }
  )

  motionCheck() {
    const MaxX = Math.max(this.motionX, this.start_x);
    const MaxY = Math.max(this.motionY, this.start_y);
    const MinX = Math.min(this.motionX, this.start_x);
    const MinY = Math.min(this.motionY, this.start_y);
    for (const item of this.drawArray) {
      if (item.x > MinX && item.x < MaxX && item.y > MinY && item.y < MaxY) {
        item.enabled = true;
      } else if (item.x + item.width > MinX && item.x + item.width < MaxX && item.y + item.height > MinY && item.y + item.height < MaxY) {
          item.enabled = true;
      } else if (item.x + item.width > MinX && item.x + item.width < MaxX && item.y > MinY && item.y < MaxY) {
          item.enabled = true;
      } else if (item.x > MinX && item.x < MaxX && item.y + item.height > MinY && item.y + item.height < MaxY) {
          item.enabled = true;
      } else if (item.x < this.start_x && item.y < this.start_y && item.x + item.width > this.start_x
          && item.y + item.height > this.start_y) {
        item.enabled = true;
      }
    }
  }

  initDrawer(width: number, height: number) {
    this.normalHeight = height;
    this.normalWidth = width;
    this.initPorportion();
    this.clearDrawArray();
  }

  initMotionDrawer(xNum: number, yNum: number, mapString: string) {
    this.clearDrawArray();
    this.normalHeight = this.canvasDraw.nativeElement.height;
    this.normalWidth = this.canvasDraw.nativeElement.width;
    this.initPorportion();
    this.motionXCount = Math.ceil(xNum / 4) * 4;
    this.motionYCount = yNum;
    const motionHeight = this.canvasDraw.nativeElement.height / yNum;
    const motionWidth = this.canvasDraw.nativeElement.width / xNum;
    for (let y = 0; y < this.motionYCount; y++) {
      for (let x = 0; x < this.motionXCount; x++) {
        const cell = {
          enabled: false,
          x: x * motionWidth,
          y: y * motionHeight,
          content: '',
          width: motionWidth,
          height: motionHeight,
          shadow: false
        };
        this.drawArray.push(cell);
      }
    }
    let tst = '';
    let progressNumber = 0;
    for (const char of mapString) {
      const mapPart = this.pfs.hex2DecimalOne(char);
      for (const pchar of mapPart) {
        tst += pchar;
        this.drawArray[progressNumber].enabled = Boolean(Number(pchar));
        progressNumber += 1;
      }
    }
  }

  getMotionString() {
    let rst: string = '';
    let patChar: string = '';
    for (let z = 0; z < this.motionYCount; z++) {
      for (let i = 0; i < this.motionXCount / 4; i++) {
        patChar = '';
        for (let j = 0; j < 4; j++) {
          patChar += String(Number(this.drawArray[i * 4 + z * this.motionXCount + j].enabled));
        }
        rst += this.pfs.decimal2Hex4(patChar);
      }
    }
    return rst;
  }

  initPorportion() {
    this.drawerHeightProportion = this.canvasDraw.nativeElement.height / this.normalHeight;
    this.drawerWidthProportion = this.canvasDraw.nativeElement.width / this.normalWidth;
  }

  clearDrawArray() {
    this.drawArray = [];
  }

  pushDrawArray(
    enabledPara: boolean, xPara: number, yPara: number,
    contentPara: string, widthPara: number, heightPara: number,
    shadowEnable: boolean = false) {
    const cell = {
      enabled: enabledPara,
      // x: this.formatX((xPara - widthPara / 2) * this.drawerWidthProportion, widthPara * this.drawerWidthProportion),
      // y: this.formatY((yPara - heightPara / 2) * this.drawerHeightProportion, heightPara * this.drawerHeightProportion),
      x: this.formatX(xPara * this.drawerWidthProportion, widthPara * this.drawerWidthProportion),
      y: this.formatY(yPara * this.drawerHeightProportion, heightPara * this.drawerHeightProportion),
      content: contentPara,
      width: widthPara * this.drawerWidthProportion,
      height: heightPara * this.drawerHeightProportion,
      shadow: shadowEnable
    };
    this.drawArray.push(cell);
  }

  pushCharArray(enabledPara: boolean, xPara: number, yPara: number, contentPara: string, shadowEnable: boolean = false) {
    const len = contentPara.length;
    const widthPara = this.calContentWidth(contentPara);
    const heightPara = this.calContentHeight();
    const cell = {
      enabled: enabledPara,
      x: this.formatX(xPara * this.drawerWidthProportion, widthPara * this.drawerWidthProportion),
      y: this.formatY(yPara * this.drawerHeightProportion, heightPara * this.drawerHeightProportion),
      content: contentPara,
      width: widthPara * this.drawerWidthProportion,
      height: heightPara * this.drawerHeightProportion,
      shadow: shadowEnable
    };
    this.drawArray.push(cell);
  }

  setCharArray(index: number, newContent: string) {
    let newWidth = this.calContentWidth(newContent) * this.drawerWidthProportion;
    newWidth = newWidth > this.canvasDraw.nativeElement.width ? this.canvasDraw.nativeElement.width : newWidth;
    if (this.drawArray[index]) {
      this.drawArray[index].content = newContent;
      this.drawArray[index].width = newWidth;
      this.drawArray[index].x = this.formatX(this.drawArray[index].x, newWidth);
    } else {
      this.logger.error('setCharArray:' + index + ' not exist in drawArray!');
    }
  }

  setFontSize(sizeNum: number) {
    this.charLength = Math.ceil(sizeNum * this.drawerHeightProportion);
    if (this.drawArray && this.drawArray.length > 0) {
      for (const item of this.drawArray) {
        if (item.content.length > 0) {
          item.height = this.calContentHeight();
          item.width = this.calContentWidth(item.content);
        }
      }
    }
  }

  calContentWidth(content: string): number {
    return content.length * (this.charLength);
  }

  get maxContentLenght(): number {
    return Math.ceil(this.canvasDraw.nativeElement.width / this.drawerWidthProportion / (this.charLength));
  }

  calContentHeight() {
    return this.charLength + 6;
  }

  getDrawArray(index: number) {
    if (this.drawArray && this.drawArray[index]) {
      if (this.drawArray[index].width < 0) {
        this.drawArray[index].x += this.drawArray[index].width;
        this.drawArray[index].width *= -1;
      }
      if (this.drawArray[index].height < 0) {
        this.drawArray[index].y += this.drawArray[index].height;
        this.drawArray[index].height *= -1;
      }
      const cell = {
        width: Math.ceil(this.drawArray[index].width / this.drawerWidthProportion),
        height: Math.ceil(this.drawArray[index].height / this.drawerHeightProportion),
        // x: this.drawArray[index].x / this.drawerWidthProportion + this.drawArray[index].width / 2,
        // y: this.drawArray[index].y / this.drawerHeightProportion + this.drawArray[index].height / 2,
        x: Math.ceil(this.drawArray[index].x / this.drawerWidthProportion),
        y: Math.ceil(this.drawArray[index].y / this.drawerHeightProportion),
      };
      return cell;
    } else {
      return null;
    }
  }

  setWidth(index: number, width: number) {
    this.drawArray[index].width = width * this.drawerWidthProportion;
  }

  drawPic() {
    if (this.pointEnabled && this.activeIndex !== 'null') {
      this.setActivePoint(this.activeIndex);
    }
    this.ctx.clearRect(0, 0, this.rect.width, this.rect.height);
    this.ctx.font = (this.charLength).toString() + 'px bold 宋体';
    this.ctx.fillStyle = '#D71920';
    this.ctx.textAlign = 'left';
    this.ctx.textBaseline = 'top';
    this.ctx.strokeStyle = '#D71920';
    this.ctx.lineWidth = 2;
    if (this.motionMode) {
      for (const item of this.drawArray) {
        if (item.enabled) {
          this.ctx.strokeRect(item.x, item.y, item.width, item.height);
        }
      }
    } else {
      if (!this.drawArray) {
        return;
      }
      for (const item of this.drawArray) {
        if (item.enabled && item.width !== 0 && item.height !== 0) {
          this.ctx.strokeRect(item.x, item.y, item.width, item.height);
          if (item.content) {
            this.ctx.fillText(item.content, item.x + 3, item.y + 3);
          }
          if (item.shadow) {
            this.ctx.fillStyle = 'rgba(0, 0, 0,0.5)';
            this.ctx.fillRect(item.x, item.y, item.width, item.height);
            this.ctx.fillStyle = '#D71920';
          }
        }
      }
      if (this.pointEnabled && this.activeIndex !== 'none') {
        for (const item of this.activePoint) {
          this.ctx.strokeRect(item.x, item.y, 5, 5);
          this.ctx.fillRect(item.x, item.y, 5, 5);
        }
      }
    }
  }

  formatX(num: number, width: number) {
    if (num < 0) {
      return 0;
    } else {
      if (num + width > this.canvasDraw.nativeElement.width) {
        return (this.canvasDraw.nativeElement.width - width > 0) ? (this.canvasDraw.nativeElement.width - width) : 0;
      } else {
        return num;
      }
    }
  }

  formatY(num: number, height: number) {
    if (num < 0) {
      return 0;
    } else {
      if (num + height > this.canvasDraw.nativeElement.height) {
        return this.canvasDraw.nativeElement.height - height;
      } else {
        return num;
      }
    }
  }

  clickOnWhich(drawerX: number, drawerY: number) {
    for (const i in this.drawArray) {
      if (this.drawArray[i].width > 0 && this.drawArray[i].height > 0) {
        if (drawerX >= this.drawArray[i].x &&
          drawerX <= this.drawArray[i].x + this.drawArray[i].width &&
          drawerY >= this.drawArray[i].y &&
          drawerY <= this.drawArray[i].y + this.drawArray[i].height) {
            return i;
          }
      }
    }
    return 'none';
  }

  checkDrawEnable() {
    for (const i in this.drawArray) {
      if (this.drawArray[i].width == 0 && this.drawArray[i].height == 0) {
        return i;
      }
    }
    return 'none';
  }

  clickOnWhichPoint(drawerX: number, drawerY: number) {
    for (const i in this.activePoint) {
      if (drawerX >= this.activePoint[i].x - 1 &&
        drawerX <= this.activePoint[i].x + 6 &&
        drawerY >= this.activePoint[i].y - 1 &&
        drawerY <= this.activePoint[i].y + 6) {
          return i;
        }
    }
    return 'none';
  }

  setZero4ActivePoint() {
    for (const item of this.activePoint) {
      item.x = 0;
      item.y = 0;
    }
  }

  setActivePoint(index: any) {
    if (index === 'none') {
      return null;
    } else {
      const loX = this.drawArray[index].x;
      const loY = this.drawArray[index].y;
      const loW = this.drawArray[index].width;
      const loH = this.drawArray[index].height;
      this.activePoint = [
        {
          x: loX,
          y: loY,
        },
        {
          x: loX + loW / 2,
          y: loY,
        },
        {
          x: loX + loW,
          y: loY,
        },
        {
          x: loX + loW,
          y: loY + loH / 2,
        },
        {
          x: loX + loW,
          y: loY + loH,
        },
        {
          x: loX + loW / 2,
          y: loY + loH,
        },
        {
          x: loX,
          y: loY + loH,
        },
        {
          x: loX,
          y: loY + loH / 2,
        },
      ];
      for (let i = 0; i < 8; i++) {
        this.activePoint[i].x -= 2;
        this.activePoint[i].y -= 2;
      }
    }
  }

  pointSetRect(rectIndex: number) {
    this.drawArray[rectIndex].x = this.activePoint[0].x + 2;
    this.drawArray[rectIndex].y = this.activePoint[0].y + 2;
    this.drawArray[rectIndex].width = this.activePoint[2].x - this.activePoint[0].x;
    this.drawArray[rectIndex].height = this.activePoint[6].y - this.activePoint[0].y;
    this.setActivePoint(rectIndex);
  }

  pointMove(index: number, deltaX: number, deltaY: number) {
    if (index == 0 || index == 4) {
      if (index == 0) {
        this.activePoint[0].x = this.noBigThan(this.activePoint[0].x + deltaX, this.activePoint[4].x);
        this.activePoint[0].y = this.noBigThan(this.activePoint[0].y + deltaY, this.activePoint[4].y);
      } else {
        this.activePoint[4].x = this.noSmallThan(this.activePoint[4].x + deltaX, this.activePoint[0].x);
        this.activePoint[4].y = this.noSmallThan(this.activePoint[4].y + deltaY, this.activePoint[0].y);
      }
      this.activePoint[this.cycleIndex(index, 8, -1)].x = this.activePoint[index].x;
      this.activePoint[this.cycleIndex(index, 8, -1)].y += deltaY / 2;
      this.activePoint[this.cycleIndex(index, 8, -2)].x = this.activePoint[index].x;
      this.activePoint[this.cycleIndex(index, 8, 1)].y = this.activePoint[index].y;
      this.activePoint[this.cycleIndex(index, 8, 1)].x += deltaX / 2;
      this.activePoint[this.cycleIndex(index, 8, 2)].y = this.activePoint[index].y;
    } else {
      if (index == 2 || index == 6) {
        if (index == 2) {
          this.activePoint[2].x = this.noSmallThan(this.activePoint[2].x + deltaX, this.activePoint[0].x);
          this.activePoint[2].y = this.noBigThan(this.activePoint[2].y + deltaY, this.activePoint[4].y);
        } else {
          this.activePoint[6].x = this.noBigThan(this.activePoint[6].x + deltaX, this.activePoint[4].x);
          this.activePoint[6].y = this.noSmallThan(this.activePoint[6].y + deltaY, this.activePoint[0].y);
        }
        this.activePoint[this.cycleIndex(index, 8, -1)].y = this.activePoint[index].y;
        this.activePoint[this.cycleIndex(index, 8, -1)].x += deltaX / 2;
        this.activePoint[this.cycleIndex(index, 8, -2)].y = this.activePoint[index].y;
        this.activePoint[this.cycleIndex(index, 8, 1)].x = this.activePoint[index].x;
        this.activePoint[this.cycleIndex(index, 8, 1)].y += deltaY / 2;
        this.activePoint[this.cycleIndex(index, 8, 2)].x = this.activePoint[index].x;
      } else {
        if (index == 1 || index == 5) {
          if (index == 1) {
            this.activePoint[1].y = this.noBigThan(this.activePoint[1].y + deltaY, this.activePoint[4].y);
          } else {
            this.activePoint[5].y = this.noSmallThan(this.activePoint[5].y + deltaY, this.activePoint[0].y);
          }
          this.activePoint[this.cycleIndex(index, 8, -1)].y = this.activePoint[index].y;
          this.activePoint[this.cycleIndex(index, 8, 1)].y = this.activePoint[index].y;
          this.activePoint[this.cycleIndex(index, 8, 2)].y += deltaY / 2;
          this.activePoint[this.cycleIndex(index, 8, -2)].y += deltaY / 2;
        } else {
          if (index == 3 || index == 7) {
            if (index == 3) {
              this.activePoint[3].x = this.noSmallThan(this.activePoint[3].x + deltaX, this.activePoint[0].x);
            } else {
              this.activePoint[7].x = this.noBigThan(this.activePoint[7].x + deltaX, this.activePoint[4].x);
            }
            this.activePoint[this.cycleIndex(index, 8, 1)].x = this.activePoint[index].x;
            this.activePoint[this.cycleIndex(index, 8, -1)].x = this.activePoint[index].x;
            this.activePoint[this.cycleIndex(index, 8, 2)].x += deltaX / 2;
            this.activePoint[this.cycleIndex(index, 8, -2)].x += deltaX / 2;
          }
        }
      }
    }
  }

  cycleIndex(index: number, cycle: number, change: number) {
    let rst: number = Number(index) + Number(change);
    while (rst < 0) {
      rst += cycle;
    }
    while (rst >= cycle) {
      rst -= cycle;
    }
    return rst;
  }

  noBigThan(num: number, target: number) {
    if (num > target) {
      return target;
    } else {
      return num;
    }
  }

  noSmallThan(num: number, target: number) {
    if (num < target) {
      return target;
    } else {
      return num;
    }
  }

  needBetween(num: number, upTarget: number, downTarget: number) {
    if (num > upTarget) {
      return upTarget;
    } else {
      if (num < downTarget) {
        return downTarget;
      } else {
        return num;
      }
    }
  }

  RectMove(index: number, deltaX: number, deltaY: number) {
    this.drawArray[index].x = this.formatX(this.drawArray[index].x + deltaX, this.drawArray[index].width);
    this.drawArray[index].y = this.formatY(this.drawArray[index].y + deltaY, this.drawArray[index].height);
  }

  drawWhenResize() {
    for (const item of this.drawArray) {
      item.x /= this.drawerWidthProportion;
      item.y /= this.drawerHeightProportion;
      item.width /= this.drawerWidthProportion;
      item.height /= this.drawerHeightProportion;
    }
    this.initPorportion();
    for (const item of this.drawArray) {
      item.x *= this.drawerWidthProportion;
      item.y *= this.drawerHeightProportion;
      item.width *= this.drawerWidthProportion;
      item.height *= this.drawerHeightProportion;
    }
    this.setActivePoint(this.activeIndex);
    this.drawPic();
  }

  resizeCanvas(aspect: number) {
    if (this.isMenuEnabled) {
      this.resizeWithMenu(aspect);
    } else {
      this.resizeCanvasFunc(aspect);
    }
  }

  resizeWithMenu(aspect: number) {
    if (!this.video || this.videoPara.height === 0 || this.videoPara.width === 0) {
      return;
    }
    const sideBarHeight = this.los.sideBarHeightValue - 40;
    const boxWidth = this.canvasBox.nativeElement.clientWidth;
    let setHeight = this.videoPara.height;
    let setWidth = this.videoPara.width;
    if (setWidth !== boxWidth) {
      setHeight = Math.ceil(setHeight * boxWidth / setWidth);
      setWidth = boxWidth;
    }
    if (setHeight > sideBarHeight) {
      setWidth = Math.ceil(setWidth * sideBarHeight / setHeight);
      setHeight = sideBarHeight;
    }
    this.Re2.setStyle(this.videoEdgeChild.nativeElement, 'width', setWidth + 'px');
    this.Re2.setStyle(this.videoEdgeChild.nativeElement, 'height', setHeight + 'px');
    this.Re2.setStyle(this.el.nativeElement.querySelector('.player-control-menu'), 'width', setWidth + 'px');
    this.Re2.setStyle(this.canvasDraw.nativeElement, 'left', this.videoEdgeChild.nativeElement.offsetLeft + 'px');
    this.Re2.setStyle(this.canvasDraw.nativeElement, 'top', this.videoEdgeChild.nativeElement.offsetTop + 'px');
    this.rect = this.canvasDraw.nativeElement.getBoundingClientRect();
    this.size.height = setHeight;
    this.size.width = setWidth;
  }

  resizeCanvasFunc(aspect: number): void {
    const sideBarHeight = this.los.sideBarHeightValue;
    let boxWidth = this.canvasBox.nativeElement.clientWidth;
    if (boxWidth * aspect > sideBarHeight && !this.isMenuEnabled) {
      boxWidth = Math.round((sideBarHeight * 0.8) / aspect);
    }
    this.Re2.setStyle(this.el.nativeElement.querySelector('.player-control-menu'), 'width', boxWidth + 'px');
    if (this.isRtmp) {
      this.Re2.setStyle(this.el.nativeElement.querySelector('.canvasvideo'), 'display', 'none');
      this.Re2.setStyle(this.el.nativeElement.querySelector('.videoEdge'), 'display', 'block');
      this.videoEdgeChild.nativeElement.width = boxWidth;
      this.videoEdgeChild.nativeElement.height = boxWidth * aspect;
      this.videoChild.nativeElement.width = boxWidth;
      this.videoChild.nativeElement.height = boxWidth * aspect;
      const adjustList = ['.videoEdge', '.big-play-btn-area'];
      for (const className of adjustList) {
        const adjustElement = this.el.nativeElement.querySelector(className);
        this.Re2.setStyle(adjustElement, 'width', boxWidth + 'px');
        this.Re2.setStyle(adjustElement, 'height', boxWidth * aspect + 'px');
      }
    } else {
      this.Re2.setStyle(this.el.nativeElement.querySelector('.canvasvideo'), 'display', 'block');
      this.Re2.setStyle(this.el.nativeElement.querySelector('.videoEdge'), 'display', 'none');
      this.canvasVideo.nativeElement.width = boxWidth;
      this.canvasVideo.nativeElement.height = boxWidth * aspect;
      const btnElement = this.el.nativeElement.querySelector('.big-play-btn-area');
      if (btnElement) {
        this.Re2.setStyle(btnElement, 'width', boxWidth + 'px');
        this.Re2.setStyle(btnElement, 'height', boxWidth * aspect + 'px');
      }
    }
    this.canvasDraw.nativeElement.width = boxWidth;
    this.canvasDraw.nativeElement.height = boxWidth * aspect;
    if (this.isRtmp) {
      this.Re2.setStyle(this.canvasDraw.nativeElement, 'left', this.videoEdgeChild.nativeElement.offsetLeft + 'px');
      this.Re2.setStyle(this.canvasDraw.nativeElement, 'top', this.videoEdgeChild.nativeElement.offsetTop + 'px');
    } else {
      this.Re2.setStyle(this.canvasDraw.nativeElement, 'left', this.canvasVideo.nativeElement.offsetLeft + 'px');
      this.Re2.setStyle(this.canvasDraw.nativeElement, 'top', this.canvasVideo.nativeElement.offsetTop + 'px');
    }
    this.rect = this.canvasDraw.nativeElement.getBoundingClientRect();
    this.size.height = boxWidth * aspect;
    this.size.width = boxWidth;
  }

  reshapeCanvas() {
    this.resizeCanvas(this.aspect);
    this.ctx = this.canvasDraw.nativeElement.getContext('2d');
  }

  @HostListener('window:resize', ['$event'])
  onResize(event): void {
    if (this.isViewInit) {
      this.resizeCanvas(this.aspect);
      if (this.drawerHeightProportion) {
        this.drawWhenResize();
      }
      this.draw4ImageMode();
    }
  }

  setPlay(url: string) {
    if (url.startsWith('RTMP') || url.startsWith('RTSP') || url.startsWith('rtmp') || url.startsWith('rtsp')) {
      this.isRtmp = true;
      this.resizeCanvasFunc(this.aspect);
      this.videoPlay(url);
    } else {
      this.isRtmp = false;
      this.resizeCanvasFunc(this.aspect);
      this.canvasPlay(url);
    }
  }

  videoPlay(url: string) {
    const options: any = {
      controls: true,
      controlBar: {
        pictureInPictureToggle: false,
        progressControl: false,
        remainingTimeDisplay: false,
      },
      autoplay: false,
      preload: 'auto',
      fluid: true,
      flash: {
        swf: 'assets/video-js.swf',
      },
      techOrder: ['html5', 'flash'],
    };
    this.video = videojs(this.videoChild.nativeElement, options);
    this.video.ready(
      () => {
        this.video.play();
        if (!this.isIe) {
          this.videoPara.height = this.video.height();
          this.videoPara.width = this.video.width();
        }
        this.videoPara.isReady = true;
        this.isPlaying = true;
        if (this.isMenuEnabled) {
          this.resizeWithMenu(this.aspect);
        }
      }
    );
    this.video.on('ended',
      () => {
        this.isPlaying = false;
      }
    );
    this.video.on('pause', () => {
      if (this.isPlaying) {
        this.tips.setRbTip('playerPauseTip');
      }
      this.isPlaying = false;
    });
    this.video.on('play', () => {
      this.isPlaying = true;
    });
    this.resizeCanvas(this.aspect);
    if (this.videoBigPlayShowStatus) {
      this.usingDIYMenu(false);
      const vjsPlayBtn = this.el.nativeElement.querySelector('.vjs-big-play-button');
      if (vjsPlayBtn) {
        this.Re2.setStyle(vjsPlayBtn, 'display', 'none');
        this.videoBigPlayShowStatus = false;
      }
    }
    if (url) {
      this.video.src({
        src: url,
        type: 'rtmp/flv',
      });
      this.video.play();
    }
  }

  canvasPlay(url: string) {
    this.isRtmp = false;
    if (!this.player) {
      this.player = new Player();
    }
    // this.player.play('https://roblin.cn/wasm/video/h265_high.mp4', this.canvasVideo, function (e) {
    this.player.play(url, this.canvasVideo, function(e) {
    // this.player.play('/assets/json/sub_20200314035335_1.mp4', this.canvasVideo, function(e) {
      this.logger.error('play error ' + e.error + ' status ' + e.status + '.');
      if (e.error === 1) {
        this.player.logger.info('finished');
        this.player.stop();
      }
    }, 512 * 1024, false);
    // 9212000
  }

  onAdd(): void {
    this.isDrawing = !this.isDrawing;
  }

  onDel() {
    this.tips.showCTip('isDeleteAllMask');
    this.ctOb = this.tips.ctAction.subscribe(
      change => {
        if (change === 'onYes') {
          this.deleteAllMask();
          this.tips.setCTPara('close');
        } else if (change === 'onNo') {
          this.ctOb.unsubscribe();
          this.ctOb = null;
        }
      }
    );
  }

  deleteAllMask() {
    if (this.motionMode) {
      for (const item of this.drawArray) {
        item.enabled = false;
      }
    } else {
      for (const item of this.drawArray) {
        item.x = 0;
        item.y = 0;
        item.width = 0;
        item.height = 0;
      }
      this.setZero4ActivePoint();
      this.activeIndex = 'none';
    }
    this.drawPic();
    // this.player.stop();
  }

  changeSoundStatus() {
    this.isMute = !this.isMute;
  }

  usingDIYMenu(judge: boolean = true) {
    if (judge) {
      const diyMenu = this.el.nativeElement.querySelector('.player-control-menu');
      if (diyMenu) {
        this.isMenuEnabled = true;
        this.Re2.setStyle(diyMenu, 'display', 'block');
      }
    } else {
      const vjMenu = this.el.nativeElement.querySelector('.vjs-control-bar');
      if (vjMenu) {
        this.Re2.setStyle(vjMenu, 'display', 'none');
      }
    }
  }

  diyPlay() {
    this.isPlaying = true;
    if (this.isRtmp) {
      this.video.play();
    } else {
      this.player.resume();
    }
  }

  diyPause() {
    this.isPlaying = false;
    if (this.isRtmp) {
      this.video.pause();
    } else {
      if (this.player) {
        this.player.pause();
      }
    }
  }

  diyStop() {
    this.isPlaying = false;
    if (this.isRtmp) {
      return;
    } else {
      if (this.player) {
        this.player.stop();
      }
    }
  }

  bigBtnPlay() {
    this.hideBigPlayBtn();
    if (this.imageMode) {
      this.isRtmp = false;
      this.resizeCanvas(this.aspect);
      this.draw4ImageMode();
    } else if (this.displayUrl) {
      this.setPlay(this.displayUrl);
      // this.diyPlay();
    } else {
      // h265 test by http
      // this.setPlay('https://roblin.cn/wasm/video/h265_high.mp4');

      // h265 test by wss
      // this.setPlay('wss://roblin.cn/wss/h265_high.mp4');

      // http mp4
      // this.isRtmp = true;
      // const options: any = {
      //   controls: true,
      //   controlBar: {
      //     pictureInPictureToggle: false,
      //     progressControl: false,
      //     remainingTimeDisplay: false,
      //   },
      //   autoplay: false,
      //   preload: 'auto',
      //   fluid: true,
      //   flash: {
      //     swf: 'assets/video-js.swf',
      //   },
      //   techOrder: ['html5', 'flash'],
      // };
      // this.video = videojs(this.videoChild.nativeElement, options);
      // this.video.src({
      //   src: 'http://172.16.21.43/userdata/video1/sub_20200314035335_1.mp4?view',
      //   type: 'video/mp4',
      // });
      // this.video.play();
      // const vjsPlayBtn = this.el.nativeElement.querySelector('.vjs-big-play-button');
      // if (vjsPlayBtn) {
      //   this.Re2.setStyle(vjsPlayBtn, 'display', 'none');
      // }
      // this.usingDIYMenu(false);
      // this.resizeCanvas(this.aspect);
    }
    // necessary for all
    if (this.drawArray) {
      this.drawPic();
    }
  }

  draw4ImageMode() {
    if (this.imageMode && this.displayUrl) {
      this.playerCtx = this.canvasVideo.nativeElement.getContext('2d');
      const img = new Image();
      img.src = this.displayUrl;
      const that = this;
      img.onload = () => {
        that.playerCtx.drawImage(
          img, 0, 0,
          that.canvasVideo.nativeElement.clientWidth, that.canvasVideo.nativeElement.clientHeight
          );
      };
    }
  }

  set4Preview() {
    this.usingDIYMenu(true);
    this.hideDrawer();
    const playerButton = this.el.nativeElement.querySelectorAll('.blue-btn');
    for (const btn of playerButton) {
      this.Re2.setAttribute(btn, 'hidden', 'true');
    }
    this.employee.hire(this.epBank[0]);
    this.cfg.getRecordStatus().subscribe(
      res => {
        this.resCheck.analyseRes(res, 'getRecordStatusFail');
        if (Number(res) < 0) {
          this.tips.setRbTip('recordStatusError');
          this.isRecording = false;
        } else {
          this.isRecording = Boolean(Number(res));
        }
        this.employee.numTask(this.epBank, 0, 0, 1, 0);
      },
      err => {
        this.logger.error(err, 'set4Preview:getRecordStatus:');
        this.employee.numTask(this.epBank, 0, 0, 0, 0);
      }
    );
    this.cfg.getPlanAdvancePara(0).subscribe(
      res => {
        this.isPlanEnabled = Boolean(res['iEnabled']);
        this.employee.numTask(this.epBank, 0, 1, 1, 0);
      },
      err => {
        this.logger.error(err, 'set4Preview:getPlanAdvancePara(0):');
        this.employee.numTask(this.epBank, 0, 1, 0, 0);
      }
    );
    this.employee.observeTask(this.epBank[0].name, 5000)
      .then(
        () => {
          this.checkPlanEnabled();
        }
      )
      .catch(
        () => {
          this.logger.error('set4Preview:observeTask');
          this.tips.showInitFail();
        }
    );
    this.resizeCanvas(this.defaultAspect);
  }

  hideDrawer() {
    const drawer = this.el.nativeElement.querySelector('.canvasdraw');
    if (drawer) {
      this.Re2.setStyle(drawer, 'display', 'none');
    }
  }

  postSnapSignal() {
    this.cfg.postTakePhotoSignal().subscribe(
      res => {
        this.tips.setRbTip('snapSuccess');
      },
      err => {
        this.tips.setRbTip('snapFail');
      }
    );
  }

  hideBigPlayBtn() {
    this.Re2.setStyle(this.el.nativeElement.querySelector('.big-play-btn-area'), 'display', 'none');
  }

  destroyWhenSwitch() {
    if (this.isRtmp) {
      if (this.video !== null) {
        this.video.dispose();
        this.video = null;
      }
    }
  }

  alertRecordingStatus() {
    this.lock.lock('alertRecordingStatus', true);
    this.isRecording = !this.isRecording;
    this.cfg.sendRecordSignal(this.isRecording).subscribe(
      res => {
        this.tips.setRbTip(this.getRecordingTitle(!this.isRecording));
        this.lock.unlock('alertRecordingStatus');
        this.checkPlanEnabled();
      },
      err => {
        this.logger.error(err, 'alertRecordingStatus:');
        this.tips.setRbTip('recordingFail');
        this.isRecording = !this.isRecording;
        this.lock.unlock('alertRecordingStatus');
      }
    );
  }

  checkPlanEnabled() {
    if (this.isPlanEnabled && this.isRecording) {
      this.tips.setRbTip('videoPlanEnabledRecordUseless');
    }
  }

  getRecordingTitle(key: boolean) {
    return this.recordTitleBank[key.toString()];
  }

  reshapeByResolution(height: number, width: number) {
    this.aspect = height / width;
    this.resizeCanvas(this.aspect);
  }

  expendBtnSwitch() {
    if (this.option['expendBtn']) {
      if (this.option['expendBtn'] instanceof Array) {
        for (const item of this.option['expendBtn']) {
          this.expendBtn[item.toString()] = true;
        }
      }
    }
  }

  onSave() {
    this.saveSignal.next(false);
  }

  drawWholeMask() {
    if (this.drawArray.length >= 1) {
      this.drawArray[0].x = 0;
      this.drawArray[0].y = 0;
      this.drawArray[0].width = this.canvasDraw.nativeElement.width;
      this.drawArray[0].height = this.canvasDraw.nativeElement.height;
      this.drawPic();
    }
  }

}

export interface DrawCell {
  enabled: boolean;
  x: number;
  y: number;
  content: string;
  width: number;
  height: number;
  shadow: boolean;
}
