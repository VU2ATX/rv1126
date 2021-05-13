import { Component, OnInit, AfterViewInit, OnDestroy, ViewChild, ElementRef } from '@angular/core';
import { ViewEncapsulation } from '@angular/core';
import { FormBuilder } from '@angular/forms';
import { StreamURLInterface } from './StreamURLInterface';
import { ConfigService } from '../config.service';
import Logger from '../logger';
import { ResponseErrorService } from 'src/app/shared/func-service/response-error.service';
import { TipsService } from 'src/app/tips/tips.service';

@Component({
  selector: 'app-preview',
  templateUrl: './preview.component.html',
  styleUrls: ['./preview.component.scss'],
  encapsulation: ViewEncapsulation.None
})

export class PreviewComponent implements OnInit, AfterViewInit, OnDestroy {

  @ViewChild('preview', {static: true}) videoElement: ElementRef;
  @ViewChild('player', {static: true}) playerChild: any;

  constructor(
    private cfgService: ConfigService,
    private fb: FormBuilder,
    private resError: ResponseErrorService,
    private tips: TipsService,
  ) { }

  private logger: Logger = new Logger('config');
  private urls: StreamURLInterface[];
  private src: string;
  private isViewInit: boolean = false;
  isPTZEnabled: boolean = true;
  playerOption = {
    isReshape: false,
  };

  testForm = this.fb.group({
    url: 'ws://172.16.21.151:8081',
  });

  video: any;
  me: any;
  directions = [
    'left-up', 'up', 'right-up',
    'left', 'auto', 'right',
    'left-down', 'down', 'right-down'
  ];

  operations = [
    'zoom',
    'focus',
    'iris'
  ];

  ngOnInit() {
    this.cfgService.getStreamURLInterface()
      .subscribe(
        (res: StreamURLInterface[]) => {
          this.resError.analyseRes(res);
          this.logger.debug('getStreamURLInterface:' + res[1].sURL);
          this.urls = res;
          this.src = this.urls[1].sURL;
          // this.src = 'ws://172.16.21.193:8081';
          // ws://172.16.21.151:8081
          this.playEntry();
        },
        err => {
          this.tips.setRbTip('getVideoUrlFail');
        }
      );
  }

  ngAfterViewInit() {
    this.isViewInit = true;
    this.playerChild.set4Preview();
    this.playEntry();
  }

  ngOnDestroy() {
    if (this.video) {
      this.video.dispose();
    }
    this.playerChild.diyStop();
  }

  playEntry() {
    if (this.isViewInit && this.src) {
      this.playerChild.displayUrl = this.src;
      this.playerChild.bigBtnPlay();
    }
  }

  onConnect() {
    if (this.src) {
      this.playerChild.diyStop();
    }
    if (this.video) {
      this.video.dispose();
    }
    this.src = this.testForm.value.url;
    this.playEntry();
  }
}
