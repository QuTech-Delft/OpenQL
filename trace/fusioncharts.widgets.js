/*
 FusionCharts JavaScript Library
 Copyright FusionCharts Technologies LLP
 License Information at <http://www.fusioncharts.com/license>

 @version 3.12.1
 FusionCharts JavaScript Library
 Copyright FusionCharts Technologies LLP
 License Information at <http://www.fusioncharts.com/license>

 @version 3.12.1
*/
(function(E) {
    "object" === typeof module && "undefined" !== typeof module.exports ? module.exports = E : E(FusionCharts)
})(function(E) {
    E.register("module", ["private", "modules.renderer.js-spark", function() {
        var b = this.hcLib, k = b.BLANKSTRING, d = b.pluck, a = b.pluckNumber, n = b.extend2, L = b.chartAPI, D = b.graphics.getLightColor, u = b.COMMASTRING, e = b.graphics.convertColor, C = b.getValidValue, N = Math, J = N.min, K = N.max, I = N.ceil, G = b.toRaphaelColor, Z = b.parseUnsafeString, S = b.graphics.getColumnColor, c = b.graphics.parseColor, F = b.COLOR_TRANSPARENT,
        g = b.POSITION_BOTTOM, H = b.POSITION_MIDDLE, B = b.POSITION_START, f = b.POSITION_END, r = b.HUNDREDSTRING, N=!b.CREDIT_REGEX.test(this.window.location.hostname), y = n({}, b.defaultGaugePaletteOptions), W = function() {
            var c = this.chart, v = c.config, l = v.dataLabelStyle, p = c.components.paper, g = v.valuepadding + 2, m = this.graphics.dataLabelContainer || c.graphics.datalabelsGroup, h = v.sparkValues || (v.sparkValues = {}), w = c.graphics, w = w.sparkLabels || (w.sparkLabels = {}), q = c.linkedItems.smartLabel, z = c.get("config", "animationObj"), c = z.transposeAnimDuration,
            z = z.animType, A = "[", x = "]", na = "|", ta = {
                "class": "fusioncharts-label",
                "text-anchor": f,
                fill: l.color,
                "font-size": l.fontSize,
                "font-weight": l.fontWeight,
                "font-style": l.fontStyle,
                "font-family": l.fontFamily,
                visibility: "visible"
            }, t = {
                x: 0,
                y: 0
            };
            m && m.attr({
                "clip-rect": null
            });
            t.y = .5 * v.canvasHeight + v.canvasTop;
            t.x = v.canvasLeft - g;
            if (h.openValue && h.openValue.label || w.openValue)
                w.openValue ? (w.openValue.attr({
                    text: h.openValue.label,
                    fill: h.openValue.color || ta.fill,
                    "text-anchor": f,
                    "line-height": l.lineHeight,
                    "text-bound": [l.backgroundColor,
                    l.borderColor, l.borderThickness, l.borderPadding, l.borderRadius, l.borderDash],
                    visibility: "visible"
                }), c ? w.openValue.animate(t, c, z) : w.openValue.attr(t)) : w.openValue = p.text({
                    text: h.openValue.label,
                    x: t.x,
                    y: t.y,
                    fill: h.openValue.color || ta.fill,
                    "text-anchor": f,
                    "line-height": l.lineHeight,
                    "text-bound": [l.backgroundColor, l.borderColor, l.borderThickness, l.borderPadding, l.borderRadius, l.borderDash],
                    visibility: "visible"
                }, m);
            ta["text-anchor"] = B;
            t.x = v.canvasWidth + v.canvasLeft + g;
            if (h.closeValue && h.closeValue.label ||
            w.closeValue)
                w.closeValue ? (w.closeValue.attr({
                    text: h.closeValue.label,
                    fill: h.closeValue.color || ta.fill,
                    "text-anchor": B,
                    "line-height": l.lineHeight,
                    "text-bound": [l.backgroundColor, l.borderColor, l.borderThickness, l.borderPadding, l.borderRadius, l.borderDash],
                    visibility: "visible"
                }), c ? w.closeValue.animate(t, c, z) : w.closeValue.attr(t)) : w.closeValue = p.text({
                    text: h.closeValue.label,
                    x: t.x,
                    y: t.y,
                    fill: h.closeValue.color || ta.fill,
                    "text-anchor": B,
                    "line-height": l.lineHeight,
                    "text-bound": [l.backgroundColor, l.borderColor,
                    l.borderThickness, l.borderPadding, l.borderRadius, l.borderDash],
                    visibility: "visible"
                }, m), t.x += h.closeValue.smartObj && h.closeValue.smartObj.width + 2 + g || 0;
            if (h.highLowValue && h.highLowValue.label || w.highValue)
                h.highLowValue && h.highLowValue.label === k && (na = A = x = k), q.useEllipsesOnOverflow(v.useEllipsesWhenOverflow), q.setStyle(l), w.startBraces ? c ? w.startBraces.animate(t, c, z) : w.startBraces.attr(t) : w.startBraces = p.text({
                    text: A,
                    x: t.x,
                    y: t.y,
                    "text-anchor": B,
                    visibility: "visible"
                }, m), t.x += q.getSmartText(A).width, w.highValue ?
                (w.highValue.attr({
                    text: h.highValue.label,
                    "text-anchor": B,
                    fill: h.highValue.color || ta.fill,
                    visibility: "visible"
                }), c ? w.highValue.animate(t, c, z) : w.highValue.attr(t)) : w.highValue = p.text({
                    text: h.highValue.label,
                    x: t.x,
                    y: t.y,
                    "text-anchor": B,
                    fill: h.highValue.color || ta.fill,
                    visibility: "visible"
                }, m), t.x += q.getSmartText(h.highValue.label).width, w.separater ? c ? w.separater.animate(t, c, z) : w.separater.attr(t) : w.separater = p.text({
                    text: na,
                    x: t.x,
                    y: t.y,
                    "text-anchor": B,
                    visibility: "visible"
                }, m), t.x += q.getSmartText(na).width,
                w.lowValue ? (w.lowValue.attr({
                    text: h.lowValue.label,
                    "text-anchor": B,
                    fill: h.lowValue.color || ta.fill,
                    visibility: "visible"
                }), c ? w.lowValue.animate(t, c, z) : w.lowValue.attr(t)) : w.lowValue = p.text({
                    text: h.lowValue.label,
                    x: t.x,
                    y: t.y,
                    "text-anchor": B,
                    fill: h.lowValue.color || ta.fill,
                    visibility: "visible"
                }, m), t.x += q.getSmartText(h.lowValue.label).width, w.endBraces ? c ? w.endBraces.animate(t, c, z) : w.endBraces.attr(t) : w.endBraces = p.text({
                    text: x,
                    x: t.x,
                    y: t.y,
                    "text-anchor": B,
                    visibility: "visible"
                }, m);
            this.labelDrawn=!0
        }, p =
        function() {
            var c = this.chart, v = c.jsonData.chart, l = c.components, c = l.colorManager, p = this.components.data, l = l.xAxis[0], g = a(v.periodlength, 0), m;
            m = l.getLimit();
            l.config.band.isDraw=!0;
            g && l.setAxisConfig({
                alternateGridColor: d(v.periodcolor, c.getColor("periodColor")),
                alternateGridAlpha: d(v.periodalpha, 100),
                showAlternateGridColor: !0,
                categoryNumDivLines: p && (m.max - m.min) / g - 1,
                categoryDivLinesFromZero: 0
            });
            l.draw()
        };
        L("sparkchartbase", {
            standaloneInit: !0,
            creditLabel: N,
            showBorder: 0,
            chartTopMargin: 3,
            chartRightMargin: 3,
            chartBottomMargin: 3,
            chartLeftMargin: 3,
            canvasBorderThickness: 1,
            subTitleFontSizeExtender: 0,
            subTitleFontWeight: 0,
            defaultPaletteOptions: function(c, v) {
                var l;
                c || (c = {});
                for (l in v)
                    c[l] = v[l];
                return c
            }(n({}, y), {
                paletteColors: [["555555", "A6A6A6", "CCCCCC", "E1E1E1", "F0F0F0"], ["A7AA95", "C4C6B7", "DEDFD7", "F2F2EE"], ["04C2E3", "66E7FD", "9CEFFE", "CEF8FF"], ["FA9101", "FEB654", "FED7A0", "FFEDD5"], ["FF2B60", "FF6C92", "FFB9CB", "FFE8EE"]],
                bgColor: ["FFFFFF", "CFD4BE,F3F5DD", "C5DADD,EDFBFE", "A86402,FDC16D", "FF7CA0,FFD1DD"],
                bgAngle: [270,
                270, 270, 270, 270],
                bgRatio: ["0,100", "0,100", "0,100", "0,100", "0,100"],
                bgAlpha: ["100", "60,50", "40,20", "20,10", "30,30"],
                canvasBgColor: ["FFFFFF", "FFFFFF", "FFFFFF", "FFFFFF", "FFFFFF"],
                canvasBgAngle: [0, 0, 0, 0, 0],
                canvasBgAlpha: ["100", "100", "100", "100", "100"],
                canvasBgRatio: [k, k, k, k, k],
                canvasBorderColor: ["BCBCBC", "BEC5A7", "93ADBF", "C97901", "FF97B1"],
                toolTipBgColor: ["FFFFFF", "FFFFFF", "FFFFFF", "FFFFFF", "FFFFFF"],
                toolTipBorderColor: ["545454", "545454", "415D6F", "845001", "68001B"],
                baseFontColor: ["333333", "60634E", "025B6A",
                "A15E01", "68001B"],
                trendColor: ["666666", "60634E", "415D6F", "845001", "68001B"],
                plotFillColor: ["666666", "A5AE84", "93ADBF", "C97901", "FF97B1"],
                borderColor: ["767575", "545454", "415D6F", "845001", "68001B"],
                borderAlpha: [50, 50, 50, 50, 50],
                periodColor: ["EEEEEE", "ECEEE6", "E6ECF0", "FFF4E6", "FFF2F5"],
                winColor: ["666666", "60634E", "025B6A", "A15E01", "FF97B1"],
                lossColor: ["CC0000", "CC0000", "CC0000", "CC0000", "CC0000"],
                drawColor: ["666666", "A5AE84", "93ADBF", "C97901", "FF97B1"],
                scorelessColor: ["FF0000", "FF0000", "FF0000", "FF0000",
                "FF0000"]
            }),
            _setAxisLimits: function() {
                var c = this.components.yAxis;
                c[0] && c[0].setAxisConfig({
                    numDivLines: 0
                });
                L.mscartesian._setAxisLimits.call(this)
            },
            _fetchCaptionPos: function() {
                return - 1
            },
            _spaceManager: function() {
                var c, v = this.config, l = v.canvasBaseDepth, p = this.components.canvas.config.canvasBorderWidth;
                c = v.borderWidth;
                var g = v.canvasMarginTop, m = v.canvasMarginBottom, h = v.canvasMarginLeft, w = v.canvasMarginRight, q = v.minCanvasHeight, z = v.minCanvasWidth, A = v.height, x = v.width, na=!1, ta=!1, t = v.origCanvasTopMargin,
                pa = v.origCanvasBottomMargin, ma = v.origCanvasLeftMargin, R = v.origCanvasRightMargin;
                this._allocateSpace({
                    top: c,
                    bottom: c,
                    left: c,
                    right: c
                });
                c = .325 * v.availableHeight;
                this._getDSspace && this._allocateSpace(this._getDSspace(c));
                this._allocateSpace(this._manageActionBarSpace && this._manageActionBarSpace(.225 * v.availableHeight) || {});
                v.availableWidth = v.width;
                c = .75 * v.availableWidth;
                c = c - v.origMarginLeft - v.origMarginRight - 2 * v.borderWidth;
                c = K(.1 * v.availableWidth, c);
                this._manageChartMenuBar(c);
                c = .75 * (v.availableWidth -
                2 * v.borderWidth);
                this._placeOpenCloseValues && this._allocateSpace(this._placeOpenCloseValues(c));
                this._allocateSpace({
                    top: p,
                    bottom: p,
                    left: p,
                    right: p
                });
                this._allocateSpace({
                    bottom: l
                });
                q > A - g - m && (na=!0, p = v.canvasHeight - q, l = g + m, g = v.canvasMarginTop = p * g / l, m = v.canvasMarginBottom = p * m / l);
                z > x - h - w && (ta=!0, p = v.canvasWidth - z, l = h + w, h = v.canvasMarginLeft = p * h / l, w = v.canvasMarginRight = p * w / l);
                g = g > v.canvasTop ? g - v.canvasTop : 0;
                m = m > A - v.canvasBottom ? m + v.canvasBottom - A : 0;
                h = h > v.canvasLeft ? h - v.canvasLeft : 0;
                w = w > x - v.canvasRight ?
                w + v.canvasRight - x : 0;
                this._allocateSpace({
                    top: g,
                    bottom: m,
                    left: h,
                    right: w
                });
                na && (l = t + pa, na = v.canvasHeight, na > q && (p = na - q, g = p * t / l, m = p * pa / l), this._allocateSpace({
                    top: g,
                    bottom: m
                }));
                ta && (l = ma + R, q = v.canvasWidth, q > z && (p = q - z, h = p * ma / l, w = p * R / l), this._allocateSpace({
                    left: h,
                    right: w
                }));
                void 0 !== v.origCanvasLeftMargin && (v.canvasWidth = K(v.canvasWidth + v.canvasLeft - v.origCanvasLeftMargin, .2 * v.availableWidth), v.canvasLeft = v.origCanvasLeftMargin)
            },
            _manageCaptionSpacing: function(c) {
                var v = this.config, l = this.components, p =
                l.caption, g = l.subCaption, m = p.config, h = g.config, p = p.components, g = g.components, w = this.jsonData.chart, q = this.linkedItems.smartLabel, z = Z(w.caption), A = Z(w.subcaption), w = m.captionPadding = a(w.captionpadding, 2), x = l.chartMenuBar, x = x && x.getLogicalSpace(), na = v.height - (x && x.height || 0), ta = 0, x = 0, t = K(l.canvas.config.canvasBorderThickness, 0), pa = 0, l = {}, ma, R;
                3 < .7 * na && (w < t && (w = t + 2), m.captionPadding = h.captionPadding = w, z !== k && (R = m.style, ta = m.captionLineHeight = I(parseFloat(d(R.fontHeight, R.lineHeight), 10), 12)), A !== k &&
                (ma = h.style, x = I(parseInt(d(ma.lineHeight, ma.fontHeight), 10), 12)), q.useEllipsesOnOverflow(v.useEllipsesWhenOverflow), 0 < ta || 0 < x) && (q.setStyle(R), v = q.getSmartText(z, c, na), 0 < v.width && (v.width += 2, pa = v.height), q.setStyle(ma), ma = q.getSmartText(A, c, na - pa), 0 < ma.width && (ma.width += 2), m.captionSubCaptionGap = v.height + 0 + .2 * x, p.text = v.text, m.height = v.height, m.width = v.width, m.tooltext && (p.originalText = v.tooltext), g.text = ma.text, h.height = ma.height, h.width = ma.width, h.tooltext && (p.originalText = ma.tooltext), p = Math.max(v.width,
                ma.width), m.captionPadding = w = J(c - p, w), 0 < p && (p = J(c, p + w)), m.maxCaptionWidth = h.maxCaptionWidth = p, m.isOnLeft ? l.left = p : l.right = p);
                return l
            },
            _manageCaptionPosition: function() {
                var c = this.config, v = this.components, l = v.caption.config, v = v.subCaption.config, p = l.captionPosition, a = K(l.width, v.width), m = c.borderWidth || 0, h = c.height, w = (c.sparkValues || {}).openValueWidth || 0, q = l.captionPadding, z = l.captionSubCaptionGap;
                switch (p) {
                case H:
                    l.y = (h - (l.height + v.height)) / 2 + .5 * (l._offsetHeight || 0);
                    break;
                case g:
                    l.y = h - (l.height +
                    v.height) - c.marginBottom - m;
                    break;
                default:
                    l.y = c.marginTop + m + (l._offsetHeight || 0)
                }
                v.y = l.y + z;
                l.isOnLeft ? (l.align = v.align = f, l.x = v.x = c.canvasLeft - (m + w + 2 + q) + c.borderWidth) : (l.align = v.align = B, l.x = v.x = c.width - c.marginRight - a - m)
            }
        }, L.sscartesian);
        L("sparkcolumn", {
            standaloneInit: !0,
            creditLabel: N,
            friendlyName: "sparkcolumn Chart",
            defaultDatasetType: "sparkcolumn",
            _parseCanvasCosmetics: function() {
                var c, v, l = this.config;
                v = this.components;
                var p = v.canvas.config, g = this.jsonData.chart;
                v = v.colorManager;
                var m, h = this.is3D ?
                b.chartPaletteStr.chart3D: b.chartPaletteStr.chart2D, w = p.isRoundEdges = a(g.useroundedges, 0);
                c = a(g.showxaxisline, g.showyaxisline, 0) ? 0 : 1;
                p.canvasBorderRadius = a(g.plotborderradius, w ? 2 : 0);
                c = p.showCanvasBorder=!!a(g.showcanvasborder, c, void 0, w ? 0 : 1);
                m = p.oriCanvasBorderThickness = K(a(g.canvasborderthickness, w ? 0 : a(this.canvasborderthickness, 1), 0));
                p.canvasBorderWidth = this.is3D ? 0 : c ? m : 0;
                p.canvasBorderColor = e(d(g.canvasbordercolor, v.getColor("canvasBorderColor")), d(g.canvasborderalpha, v.getColor("canvasBorderAlpha")));
                c = p.canBGAlpha = d(g.canvasbgalpha, v.getColor("canvasBgAlpha"));
                p.canBGColor = {
                    FCcolor: {
                        color: d(g.canvasbgcolor, v.getColor(h.canvasBgColor)),
                        alpha: d(g.canvasbgalpha, 100),
                        angle: d(g.canvasbgangle, 0),
                        ratio: d(g.canvasbgratio)
                    }
                };
                v = p.shadow = a(g.showshadow, w, 0) && w ? {
                    enabled: !0,
                    opacity: c / 100
                } : 0;
                p.shadowOnCanvasFill = v && v.enabled;
                l.origMarginTop = a(g.charttopmargin, 3);
                l.origMarginLeft = a(g.chartleftmargin, 3);
                l.origMarginBottom = a(g.chartbottommargin, 3);
                l.origMarginRight = a(g.chartrightmargin, 3);
                l.origCanvasLeftMargin =
                a(g.canvasleftmargin);
                l.origCanvasRightMargin = a(g.canvasrightmargin);
                l.origCanvasTopMargin = a(g.canvastopmargin);
                l.origCanvasBottomMargin = a(g.canvasbottommargin);
                p.canvasPadding = a(g.canvaspadding, 0);
                p.origCanvasTopPad = a(g.canvastoppadding, 0);
                p.origCanvasBottomPad = a(g.canvasbottompadding, 0);
                p.origCanvasLeftPad = a(g.canvasleftpadding, 0);
                p.origCanvasRightPad = a(g.canvasrightpadding, 0)
            },
            canvasBorderThickness: 1,
            singleseries: !1
        }, L.sparkchartbase, {
            showplotborder: 0,
            enablemousetracking: !0
        });
        E.register("component",
        ["dataset", "sparkcolumn", {
            _setConfigure: function(c) {
                var p = this.chart, l = p.jsonData.chart, g = this.config, b = p.config, m = this.JSONData, h = c || m.data, w = h && h.length, q = p.config.categories, q = q && q.length;
                c = c && c.data.length || J(q, w);
                var w = b.plothovereffect, z = p.components.colorManager, b = b.useroundedges, A, x, na, ta, t, pa, ma, R, Aa, f, y, H, M, F, B, W, n;
                g.plotgradientcolor = k;
                g.showvalues = a(m.showvalues, l.showvalues, 0);
                g.showShadow = a(l.showshadow, 0);
                this.__base__._setConfigure.call(this);
                p = this.components.data;
                q = g.maxValue;
                n =
                g.minValue;
                z = d(l.plotfillcolor, z.getColor("plotFillColor"));
                R = d(l.plotfillalpha, r);
                Aa = d(l.plotborderalpha, r);
                f = d(l.plotbordercolor, z);
                y = d(l.highcolor, "000000");
                H = d(l.lowcolor, "000000");
                M = d(l.highbordercolor, l.plotbordercolor, y);
                F = d(l.lowbordercolor, l.plotbordercolor, H);
                for (W = 0; W < c; W++)
                    if (B = h[W], ta = p[W], ma = ta.config, A = z, x = f, ma.setValue == q && (A = y, x = M), ta.config.setValue == n && (A = H, x = F), ma.colorArr = A = S(A + u + g.plotgradientcolor, R, "0", "90", b, x, Aa, 0, 0), 0 !== w && A) {
                        x = na = void 0;
                        ma.setValue == q && (x = l.highhovercolor,
                        na = l.highhoveralpha);
                        ta.config.setValue == n && (x = l.lowhovercolor, na = l.lowhoveralpha);
                        x = d(B.hovercolor, m.hovercolor, x, l.plotfillhovercolor, l.columnhovercolor, A[0].FCcolor.color);
                        x = x.split(/\s{0,},\s{0,}/);
                        ta = x.length;
                        for (t = 0; t < ta; t++)
                            x[t] = D(x[t], 70);
                            x = x.join(",");
                            na = d(B.hoveralpha, m.hoveralpha, na, l.plotfillhoveralpha, l.columnhoveralpha, R);
                            ta = d(B.hovergradientcolor, m.hovergradientcolor, l.plothovergradientcolor, g.plotgradientcolor);
                            !ta && (ta = k);
                            t = d(B.borderhovercolor, m.borderhovercolor, l.plotborderhovercolor,
                            g.plotbordercolor);
                            pa = d(B.borderhoveralpha, m.borderhoveralpha, l.plotborderhoveralpha, l.plotfillhoveralpha, Aa, R);
                            a(B.borderhoverthickness, m.borderhoverthickness, l.plotborderhoverthickness, void 0);
                            1 == w && x === A[0].FCcolor.color && (x = D(x, 70));
                            B = S(x + u + ta, na, "0", "90", b, t, pa.toString(), 0, 0);
                            ma.setRolloutAttr = {
                                fill: G(A[0]),
                                stroke: void 0,
                                "stroke-width": void 0
                            };
                            ma.setRolloverAttr = {
                                fill: G(B[0]),
                                stroke: void 0,
                                "stroke-width": void 0
                            }
                    }
            },
            draw: function() {
                this.__base__.draw.call(this);
                p.call(this)
            }
        }, "Column"]);
        E.register("component",
        ["datasetGroup", "SparkColumn", {}, "column"]);
        L("sparkwinloss", {
            standaloneInit: !0,
            friendlyName: "sparkwinloss Chart",
            creditLabel: N,
            defaultDatasetType: "sparkwinloss",
            canvasBorderThickness: 0,
            applicableDSList: {
                sparkwinloss: !0
            },
            _setAxisLimits: function() {
                var c = this.components.yAxis;
                L.mscartesian._setAxisLimits.call(this);
                c[0] && c[0].setAxisRange({
                    min: - 1,
                    max: 1
                })
            },
            _placeOpenCloseValues: function(c) {
                var p = this.config, l = this.linkedItems.smartLabel, g = p.dataLabelStyle, r = a(g.borderThickness, 0), m = this.config.valuepadding +
                2 + r, h = 0, r = 0, w = p.sparkValues || {};
                l.useEllipsesOnOverflow(p.useEllipsesWhenOverflow);
                l.setStyle(g);
                w.openValue && w.openValue.label && (w.openValue.smartObj = l.getSmartText(w.openValue.label), h = w.openValue.smartObj.width + m);
                w.closeValue && w.closeValue.label && (w.closeValue.smartObj = l.getSmartText(w.closeValue.label), r += w.closeValue.smartObj.width + m);
                w.highValue && w.highValue.label && (w.highValue.smartObj = l.getSmartText(w.highValue.label));
                w.lowValue && w.lowValue.label && (w.lowValue.smartObj = l.getSmartText(w.lowValue.label));
                w.highLowValue && w.highLowValue.label && (w.highLowValue.smartObj = l.getSmartText(w.highLowValue.label), r += w.highLowValue.smartObj.width + m);
                p = w.openValueWidth = J(h, c);
                c = w.closeValueWidth = J(r, c - p);
                return {
                    left: p,
                    right: c
                }
            },
            _parseCanvasCosmetics: function() {
                var c, p;
                c = this.components;
                var l = this.config, g = c.canvas.config, r = this.jsonData.chart, m = c.colorManager;
                g.canvasBorderRadius = 0;
                c = g.canvasBorderThickness = 0;
                p = g.showCanvasBorder = 0;
                g.canvasBorderWidth = p ? c : 0;
                g.canvasBorderColor = e(d(r.canvasbordercolor, m.getColor("canvasBorderColor")));
                g.canBGColor = F;
                c = g.shadow = 0;
                g.shadowOnCanvasFill = c && c.enabled;
                g.origCanvasTopPad = a(r.canvastoppadding, 0);
                g.origCanvasBottomPad = a(r.canvasbottompadding, 0);
                g.origCanvasLeftPad = a(r.canvasleftpadding, 0);
                g.origCanvasRightPad = a(r.canvasrightpadding, 0);
                g.canvasPadding = 0;
                l.origCanvasLeftMargin = a(r.canvasleftmargin);
                l.origCanvasRightMargin = a(r.canvasrightmargin);
                l.origCanvasTopMargin = a(r.canvastopmargin);
                l.origCanvasBottomMargin = a(r.canvasbottommargin)
            },
            singleseries: !0
        }, L.sparkchartbase, {
            enablemousetracking: !0
        });
        E.register("component", ["dataset", "sparkwinloss", {
            _setConfigure: function(c, p) {
                var l = this.chart, g = l.config, f = this.config, m = this.JSONData, h = c || m.data, w = h && h.length, q = l.config.categories, q = q && q.length, w = c && c.data.length || J(q, w), q = l.jsonData.chart, z = l.components, A = z.colorManager, z = z.xAxis[0], x = g.showplotborder, na = f.plotColor = A.getPlotColor(this.index || this.positionIndex), ta = b.parseUnsafeString, t = d(q.plotfillcolor, A.getColor("plotFillColor")), pa = d(q.wincolor, A.getColor("winColor")), ma = d(q.losscolor, A.getColor("lossColor")),
                R = d(q.drawcolor, A.getColor("drawColor")), A = d(q.scorelesscolor, A.getColor("scorelessColor")), Aa = q.winhovercolor, y = q.losshovercolor, H = q.drawhovercolor, u = q.scorelesshovercolor, M = 0, F = 0, B = 0, W, n, e = g.plotborderthickness, S = g.isroundedges, Z = g.plothovereffect, I = f.plotfillangle, U, V, L, la, N = f.plotBorderDashStyle, O, P, Y, T, qa, E, ja, ka, Fa, Ia, ea = b.getDashStyle, va = this.components.data, ga = l.isBar, Ha = l.is3D, Ca, Ja = f.maxValue||-Infinity, Na = f.minValue || Infinity, xa;
                va || (va = this.components.data = []);
                this.__base__._setConfigure.call(this);
                f.plotgradientcolor = "";
                Y = f.showPlotBorder = a(q.showplotborder, 0);
                f.plotborderalpha = Y ? d(q.plotborderalpha, U, r) : 0;
                f.showTooltip = 0;
                for (Ca = g.showtooltip = 0; Ca < w; Ca++) {
                    c ? (O = c && c.data[Ca], xa = void 0 !== p ? p + Ca : va.length - w + Ca, n = va[xa]) : (n = va[Ca], O = h[Ca]);
                    Y = n && n.config;
                    n || (n = va[Ca] = {});
                    n.config || (Y = va[Ca].config = {});
                    switch ((O.value || "").toLowerCase()) {
                    case "w":
                        na = d(O.color, pa, t);
                        T = d(O.hovercolor, Aa, na);
                        Y.setValue = P = 1;
                        M += 1;
                        break;
                    case "l":
                        na = d(O.color, ma, t);
                        T = d(O.hovercolor, y, na);
                        Y.setValue = P =- 1;
                        F += 1;
                        break;
                    case "d":
                        na =
                        d(O.color, R, t);
                        T = d(O.hovercolor, H, na);
                        Y.setValue = P = .1;
                        B += 1;
                        break;
                    default:
                        Y.setValue = P = null
                    }
                    1 == O.scoreless && (na = d(O.color, A, t), T = d(O.hovercolor, u, O.color, A, T));
                    Y.toolText=!1;
                    Y.setLink = d(O.link);
                    Y.setDisplayValue = ta(O.displayvalue);
                    la = a(O.dashed);
                    U = a(O.dashlen, void 0);
                    qa = n = a(O.dashgap, f.plotDashGap);
                    null !== P && (Ja = K(Ja, P), Na = J(Na, P));
                    Y.plotBorderDashStyle = la = 1 === la ? ea(U, qa, e) : 0 === la ? "none" : N;
                    U = d(O.alpha, f.plotfillalpha);
                    L = d(O.alpha, f.plotborderalpha, U).toString();
                    0 > P&&!S && (W = f.plotfillAngle, I = ga ? 180 -
                    I : 360 - I);
                    Y.colorArr = P = b.graphics.getColumnColor(na + "," + f.plotgradientcolor, U, V = f.plotfillratio, I, S, f.plotbordercolor, L, ga ? 1 : 0, Ha?!0 : !1);
                    Y.label = C(ta(z.getLabel(a(xa - w, Ca)).label));
                    0 !== Z && (T = d(O.hovercolor, m.hovercolor, q.plotfillhovercolor, q.columnhovercolor, na), qa = d(O.hoveralpha, m.hoveralpha, q.plotfillhoveralpha, q.columnhoveralpha, U), E = d(O.hovergradientcolor, m.hovergradientcolor, q.plothovergradientcolor, f.plotgradientcolor), !E && (E = ""), V = d(O.hoverratio, m.hoverratio, q.plothoverratio, V), ja = a(360 - O.hoverangle,
                    360 - m.hoverangle, 360 - q.plothoverangle, I), ka = d(O.borderhovercolor, m.borderhovercolor, q.plotborderhovercolor, f.plotbordercolor), L = d(O.borderhoveralpha, m.borderhoveralpha, q.plotborderhoveralpha, L, U), U = a(O.borderhoverthickness, m.borderhoverthickness, q.plotborderhoverthickness, e), Fa = a(O.borderhoverdashed, m.borderhoverdashed, q.plotborderhoverdashed), Ia = a(O.borderhoverdashgap, m.borderhoverdashgap, q.plotborderhoverdashgap, void 0), O = a(O.borderhoverdashlen, m.borderhoverdashlen, q.plotborderhoverdashlen, n), O =
                    Fa ? ea(O, Ia, U) : la, 1 == Z && T === na && (T = D(T, 70)), n = b.graphics.getColumnColor(T + "," + E, qa, V, ja, S, ka, L.toString(), ga ? 1 : 0, Ha?!0 : !1), Y.setRolloutAttr = {
                        fill: Ha ? [G(P[0]), !g.use3dlighting]: G(P[0]),
                        stroke: x && G(P[1]),
                        "stroke-width": e,
                        "stroke-dasharray": la
                    }, Y.setRolloverAttr = {
                        fill: Ha ? [G(n[0]), !g.use3dlighting]: G(n[0]),
                        stroke: x && G(n[1]),
                        "stroke-width": U,
                        "stroke-dasharray": O
                    });
                    W && (I = W);
                    xa++
                }
                f.maxValue = 1;
                f.minValue =- 1;
                1 == a(q.showvalue, 1) && (l.config.sparkValues = {
                    closeValue: {}
                }, l.config.sparkValues.closeValue.label = M +
                "-" + F + (0 < B ? "-" + B : k))
            },
            draw: function() {
                this.__base__.draw.call(this);
                p.call(this)
            },
            drawLabel: W
        }, "column"]);
        E.register("component", ["datasetGroup", "SparkWinLoss", {
            manageSpace: function() {},
            draw: function() {
                var c = this.positionStackArr, g = c.length, p, r, f, m, h = this.chart;
                p = h.config.viewPortConfig.scaleX || 1;
                r = h.is3D;
                f = h.graphics.columnGroup;
                var w = h.graphics;
                m = h.components.canvas.config.clip["clip-canvas"].slice(0);
                w = w.datalabelsGroup;
                h = h.get("config", "animationObj").duration;
                m[2]*=p;
                f.clip || r || (f.attr({
                    "clip-rect": m
                }),
                w.attr({}));
                h ? (!r && f.animate({
                    "clip-rect": m
                }, h, "normal"), !r && w.animate({}, h, "normal")) : (!r && f.attr({
                    "clip-rect": m
                }), !r && w.attr({}));
                this.preDrawCalculate();
                this.drawSumValueFlag=!0;
                for (p = 0; p < g; p++)
                    for (r = c[p], f = r.length, this.manageClip=!0, r = 0; r < f; r++)
                        m = c[p][r].dataSet, m.draw()
            }
        }, "column"]);
        L("sparkline", {
            standaloneInit: !0,
            friendlyName: "SparkLine Chart",
            creditLabel: N,
            defaultDatasetType: "sparkline",
            singleseries: !0,
            showValues: 0,
            _parseCanvasCosmetics: L.sparkwinloss._parseCanvasCosmetics,
            _placeOpenCloseValues: L.sparkwinloss._placeOpenCloseValues,
            defaultPlotShadow: 0,
            axisPaddingLeft: 0,
            axisPaddingRight: 0,
            applicableDSList: {
                line: !0
            }
        }, L.sparkchartbase, {
            showvalues: 0,
            enablemousetracking: !0
        });
        E.register("component", ["dataset", "sparkline", {
            type: "sparkline",
            configure: function() {
                var c = this.config, p = this.JSONData, g = this.chart.jsonData.chart;
                this.__base__.configure.call(this);
                c.linethickness = a(p.linethickness, g.linethickness, 1)
            },
            _setConfigure: function(p) {
                var g = this.chart, l = this.config, r = this.JSONData, f = g.jsonData.chart, r = p || r.data, m, h = g.components.xAxis[0];
                p = p && p.data.length || h.getCategoryLen();
                var w = g.components.colorManager, g = g.config, q, z, h = c(d(f.opencolor, "0099FF")), A = c(d(f.closecolor, "0099FF")), x = c(d(f.highcolor, "00CC00")), na = c(d(f.lowcolor, "CC0000")), ta = c(d(f.anchorcolor, w.getColor("plotFillColor"))), t = a(f.showopenanchor, f.drawanchors, f.showanchors, 1), pa = a(f.showcloseanchor, f.drawanchors, f.showanchors, 1), ma = a(f.showhighanchor, f.drawanchors, f.showanchors, 1), R = a(f.showlowanchor, f.drawanchors, f.showanchors, 1), Aa = a(f.anchoralpha, 100), b = a(f.drawanchors,
                f.showanchors, 0) ? a(f.anchoralpha, 100): 0, y = d(f.linecolor, w.getColor("plotFillColor")), H, M, G, F, u, B;
                this.__base__._setConfigure.call(this);
                w = this.components.data;
                M = l.maxValue;
                G = l.minValue;
                l.shadow = {
                    opacity: a(f.showshadow, 0) ? l.alpha / 100: 0
                };
                m = w[0];
                q = m.config;
                q.anchorProps.bgColor = d(m.anchorbgcolor, h);
                q.anchorProps.enabled=!!t;
                q.anchorProps.bgAlpha = t ? F : 0;
                q.anchorProps.enabled && q.hoverEffects && (q.hoverEffects.anchorColor = d(f.openhovercolor, f.anchorhovercolor, f.plotfillhovercolor, D(h, 70)), q.hoverEffects.anchorBgAlpha =
                a(f.openhoveralpha, f.anchorhoveralpha, f.plotfillhoveralpha, 100));
                t = q.displayValue;
                m = w[p - 1];
                q = m.config;
                q.anchorProps.bgColor = d(m.anchorbgcolor, A);
                q.anchorProps.enabled=!!pa;
                q.anchorProps.bgAlpha = pa ? F : 0;
                q.anchorProps.enabled && q.hoverEffects && (q.hoverEffects.anchorColor = d(f.closehovercolor, f.anchorhovercolor, f.plotfillhovercolor, D(A, 70)), q.hoverEffects.anchorBgAlpha = a(f.closehoveralpha, f.anchorhoveralpha, f.plotfillhoveralpha, 100));
                pa = q.displayValue;
                for (z = 0; z < p; z++)
                    m = w[z], q = m.config, m = r[z], F = a(m.anchorbgalpha,
                    Aa), l.maxRadius =- Infinity, 0 !== z && z !== p - 1 && (q.anchorProps.bgColor = d(m.anchorbgcolor, ta), q.anchorProps.bgAlpha = a(m.anchorbgalpha, b), q.hoverEffects.anchorColor = c(d(f.anchorhovercolor, f.plotfillhovercolor, D(y, 70))), q.hoverEffects.anchorBgAlpha = a(f.lowhoveralpha, f.anchorhoveralpha, f.plotfillhoveralpha, 100)), q.anchorProps.radius = a(f.anchorradius, m.anchorradius, 2), q.anchorProps.borderThickness = 0, q.hoverEffects.anchorBorderThickness = 0, q.hoverEffects.anchorRadius = a(f.anchorhoverradius, f.anchorradius, m.anchorradius,
                    3), l.maxRadius = Math.max(q.anchorProps.radius + q.anchorProps.borderThickness / 2, l.maxRadius), q.setValue === G && (q.anchorProps.bgColor = d(m.anchorbgcolor, na), q.hoverEffects.anchorColor = d(f.lowhovercolor, f.anchorhovercolor, f.plotfillhovercolor, D(na, 70)), q.hoverEffects.anchorBgAlpha = a(f.lowhoveralpha, f.anchorhoveralpha, f.plotfillhoveralpha, 100), q.anchorProps.enabled=!!R, q.anchorProps.bgAlpha = R ? F : 0, B = q.displayValue), q.setValue === M && (q.anchorProps.bgColor = d(m.anchorbgcolor, x), q.hoverEffects.anchorColor = d(f.highhovercolor,
                    f.anchorhovercolor, f.plotfillhovercolor, D(x, 70)), q.hoverEffects.anchorBgAlpha = a(f.highhoveralpha, f.anchorhoveralpha, f.plotfillhoveralpha, 100), q.anchorProps.enabled=!!ma, q.anchorProps.bgAlpha = ma ? F : 0, u = q.displayValue), F = q.setValue, void 0 !== F && null !== F && (H = 1);
                l = g.sparkValues = {
                    openValue: {
                        color: h
                    },
                    closeValue: {
                        color: A
                    },
                    highValue: {
                        color: x
                    },
                    lowValue: {
                        color: na
                    },
                    highLowValue: {}
                };
                H && (l.openValue.label = a(f.showopenvalue, 1) ? t : k, l.closeValue.label = a(f.showclosevalue, 1) ? pa : k, a(f.showhighlowvalue, 1) && (l.highLowValue.label =
                "[" + u + " | " + B + "]", l.highValue.label = u, l.lowValue.label = B))
            },
            draw: function() {
                this.__base__.draw.call(this);
                this._drawSparkValues();
                p.call(this)
            },
            _drawSparkValues: function() {
                W.call(this)
            }
        }, "Line"])
    }, [3, 2, 0, "sr2"]]);
    E.register("module", ["private", "modules.renderer.js-messagelogger", function() {
        var b = this.window, k = b.document, d = 8 === k.documentMode, a = this.hcLib, n = a.Raphael, L = a.componentDispose, D = a.pluckNumber, u = a.isIE, e = a.graphics.HEXtoRGB, C = a.graphics.convertColor, N = a.pluck, J = Math.min, K, I = {}, G = {
            display: "block",
            paddingLeft: "10px",
            paddingRight: "10px",
            "font-family": "Arial",
            "font-size": "11px"
        };
        I.literal = I.info = {
            title: '<span style="color: #005900">$titleVal$</span>',
            body: "<span>$msgVal$</span>"
        };
        I.link = {
            title: I.info.title,
            body: '<a href="$msgLinkVal$">$msgVal$</a>'
        };
        I.error = {
            title: '<span style="color: #CC0000">$titleVal$</span>',
            body: '<span style="color: #CC0000">$msgVal$</span>'
        };
        K = function(a, b) {
            var c = this.config = {}, d = (a.msgType || "").toLowerCase(), g = a.msgTitle, H = a.msgText, G = N(a.msgLink, H);
            c.totalHTML = "";
            this.graphics =
            {};
            this.linkedItems = {
                msgLogger: b
            };
            d = I[d] || I.literal;
            g && (c.titleHTML = d.title.replace("$titleVal$", g), c.totalHTML += c.titleHTML);
            H && (c.msgHTML = d.body.replace("$msgVal$", H), c.msgHTML = c.msgHTML.replace("$msgLinkVal$", G), c.totalHTML += c.msgHTML)
        };
        K.prototype = {
            draw: function() {
                var a = this.config, b = this.graphics, c, F = b.element, g = this.linkedItems.msgLogger, H = g.graphics, B = H && H.log && H.log.element, H = H.logWrapper && H.logWrapper.element, g = g.config;
                if (!F) {
                    F = b.element = k.createElement("span");
                    for (c in G)
                        F.style[c] = G[c];
                    B.appendChild && B.appendChild(F)
                }
                b.element.innerHTML = a.totalHTML;
                u && d && (a = B.innerHTML, B.innerHTML = a);
                g.scrollToBottom && (g.dynamicScrolling=!0, B = H.scrollHeight, H.scrollTop = B)
            },
            dispose: function() {
                var a = this.graphics, b = this.linkedItems.msgLogger;
                b && b.graphics && b.graphics.log && b.graphics.log.element && b.graphics.log.element.removeChild && b.graphics.log.element.removeChild(a.element);
                delete a.element;
                L.call(this)
            }
        };
        K.prototype.constractor = K;
        E.register("component", ["logger", "message", {
            pIndex: 1,
            customConfigFn: null,
            init: function(a) {
                var b = this.linkedItems || (this.linkedItems = {});
                this.components = this.components || {};
                this.components.messages = this.components.messages || [];
                this.graphics = this.graphics || {};
                b.chart = a
            },
            configure: function() {
                var a = this.config || (this.config = {}), b = this.linkedItems.chart, c = b.get && b.get("jsonData", "chart") || {}, d = a.usemessagelog = b.get("config", "usemessagelog");
                a.messageLogWPercent = J(D(c.messagelogwpercent, 80), 100);
                a.messageLogHPercent = J(D(c.messageloghpercent, 70), 100);
                a.messageLogShowTitle = D(c.messagelogshowtitle,
                1);
                a.messageLogTitle = N(c.messagelogtitle, "Message Log");
                a.messageLogColor = N(c.messagelogcolor, "#fbfbfb").replace(/^#?([a-f0-9]+)/ig, "$1");
                a.messageLogColorRgb = e(a.messageLogColor);
                a.messageGoesToJS = D(c.messagegoestojs, 0);
                a.messageGoesToLog = D(c.messagegoestolog, 1);
                a.messageJSHandler = N(c.messagejshandler, "");
                a.messagePassAllToJS = D(c.messagepassalltojs, 0);
                a.messagePassAsObject = D(c.messagepassasobject, 0);
                a.messageLogIsCancelable = D(c.messagelogiscancelable, 1);
                a.alwaysShowMessageLogMenu = D(c.alwaysshowmessagelogmenu,
                d);
                b.config.useShowLogMenu = d && a.messageGoesToLog;
                a.dynamicScrolling=!1;
                a.scrollToBottom=!0
            },
            _createMessage: function(a) {
                a = new K(a, this);
                this.graphics.container && a.draw();
                return a
            },
            addLog: function(a) {
                var d = this.config, c = this.components.messages, G = D(a.msgGoesToLog, d.messageGoesToLog), g = D(a.msgGoesToJS, d.messageGoesToJS), H = b[d.messageJSHandler], u = N(a.msgId, ""), f = N(a.msgTitle, ""), r = N(a.msgText, ""), y = N(a.msgType, "literal");
                d.usemessagelog && (g && H && "function" === typeof H && (d.messagePassAllToJS ? d.messagePassAsObject ?
                H(a) : H(u, f, r, y) : H(r)), "1" === a.clearLog && this.clearLog(), G && (a.msgTitle || a.msgText) && (a = this._createMessage(a), c.push(a), 1 !== c.length || d.visible || this.show()))
            },
            show: function() {
                var a = this.graphics, b = this.config;
                b.visible || (b.visible=!0, a.container || this.draw(), a.container && a.container.show())
            },
            hide: function() {
                var a = this.graphics;
                this.config.visible=!1;
                a.container && a.container.hide()
            },
            clearLog: function() {
                var a = this.components.messages, b, c = a.length;
                for (b = 0; b < c; b += 1)
                    a[b] && a[b].dispose && a[b].dispose();
                a.splice(0, c)
            },
            isDrawn: function() {
                return !!this.graphics.container
            },
            draw: function() {
                var a = this.config, b = this.components.messages, c;
                if (a.usemessagelog)
                    for (this._createHTMLDialogue(), a.visible || this.hide(), c = b.length, a = 0; a < c; a += 1)
                        b[a] && b[a].draw && b[a].draw();
                else 
                    this.isDrawn() && (this.clearLog(), this.hide())
            },
            _createHTMLDialogue: function() {
                var a = this, b = a.config, c = a.graphics, d = a.components, g = a.linkedItems.chart, H = g.get("components", "paper"), G = g.get("linkedItems", "container"), f = g.get("config"), r = f.width,
                y = f.height, u = (g = f.style) && g.inCanvasStyle, p = b.messageLogShowTitle, ia = b.messageLogIsCancelable, v = b.messageLogColor, l = b.messageLogTitle, e = d.paper, D = c.cg, m = b.messageLogWPercent / 100 * r, h = b.messageLogHPercent / 100 * y, w = (r - m) / 2, q = (y - h) / 2, z = m - 18 - 22, A = h - 18 - 22, x = w + m - 21, na = q + 3, g = c.container, f = a.isDrawn() && f.animation && f.animation.transposeAnimDuration;
                g || (g = c.container = H.html("div", {
                    fill: "transparent"
                }, {
                    fontSize: "10px",
                    lineHeight: "15px"
                }, G), c.veil = H.html("div", {
                    id: "veil",
                    fill: "000000",
                    opacity: .1
                }, void 0, g).on("click",
                function() {
                    ia && a.hide()
                }), l && p && (c.title = H.html("p", {
                    id: "Title",
                    innerHTML: l,
                    x: 5,
                    y: 5
                }, {
                    "font-weight": "bold"
                }, g)), c.dialog = H.html("div", {
                    id: "dialog",
                    strokeWidth: 1
                }, {
                    borderRadius: "5px",
                    boxShadow: "1px 1px 3px #000000",
                    "-webkit-border-radius": "5px",
                    "-webkit-box-shadow": "1px 1px 3px #000000",
                    filter: 'progid:DXImageTransform.Microsoft.Shadow(Strength=4, Direction=135, Color="#000000")'
                }, g), c.logBackground = H.html("div", {
                    id: "dialogBackground",
                    x: 0,
                    y: 0
                }, void 0, c.dialog), ia && (c.closeBtnContainer = H.html("div",
                {
                    id: "closeBtnContainer"
                }, {}, g), d.paper = e = new n("closeBtnContainer", 18, 18), e.setConfig("stroke-linecap", "round"), D = c.cg = e.group("closeGroup"), c.closeButton = e.symbol("closeIcon", 0, 0, 6, D).attr({
                    transform: "t9,9",
                    "stroke-width": 2,
                    stroke: C("999999"),
                    ishot: !0,
                    "stroke-linecap": "round",
                    "stroke-linejoin": "round"
                }).css({
                    cursor: "pointer",
                    _cursor: "hand"
                }).click(function() {
                    a.hide()
                })), c.logWrapper = H.html("div", {
                    id: "logWrapper"
                }, {
                    overflow: "auto"
                }, c.dialog).on("scroll", function() {
                    var c = this && this.scrollTop, t = this &&
                    this.scrollHeight, q = this && this.offsetHeight;
                    b.dynamicScrolling ? b.dynamicScrolling=!1 : b.scrollToBottom = t - c === q?!0 : !1
                }), c.log = H.html("div", {
                    id: "log",
                    x: 0,
                    y: 0
                }, {}, c.logWrapper));
                g.css({
                    fontFamily: u.fontFamily
                });
                c.dialog.attr({
                    fill: "ffffff",
                    stroke: v
                });
                c.logBackground.attr({
                    fill: v
                });
                d = {
                    width: r,
                    height: y
                };
                H = {
                    x: w,
                    y: q,
                    width: m,
                    height: h
                };
                G = {
                    width: m,
                    height: h
                };
                x = {
                    width: 18,
                    height: 18,
                    x: x,
                    y: na
                };
                m = {
                    x: (m - z) / 2,
                    y: (h - A) / 2,
                    width: z,
                    height: A
                };
                f ? (g.animate(d, f, "normal"), c.veil.animate(d, f, "normal"), c.dialog.animate(H, f, "normal"),
                c.logBackground.animate(G, f, "normal"), c.closeBtnContainer && c.closeBtnContainer.animate(x, f, "normal"), c.logWrapper.animate(m, f, "normal")) : (g.attr(d), c.veil.attr(d), c.dialog.attr(H), c.logBackground.attr(G), c.closeBtnContainer && c.closeBtnContainer.attr(x), c.logWrapper.attr(m))
            }
        }
        ])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-alertmanager", function() {
        var b = this, k = b.hcLib;
        E.register("component", ["manager", "alert", {
            pIndex: 1,
            init: function(b) {
                this.linkedItems = {
                    chart: b
                }
            },
            configure: function() {
                var b =
                this.linkedItems.chart, a = b.get("jsonData", "alerts"), a = a && a.alert, b = b.get("components", "numberFormatter"), n = this.config || (this.config = {}), k, D;
                if (a && a.length)
                    for (n.alertArr = a, n = a.length, k = 0; k < n; k += 1)
                        D = a[k], D.minvalue = b.getCleanValue(D.minvalue), D.maxvalue = b.getCleanValue(D.maxvalue);
                else 
                    n.alertArr = []
            },
            processRTData: function(b) {
                var a = this.linkedItems.chart.get("components", "numberFormatter"), n, k, D, u, e, C, N;
                if (b && b.dataset && b.dataset.length)
                    for (D = b.dataset.length, n = 0; n < D; n += 1)
                        if (u = b.dataset[n], u.data &&
                        u.data.length)
                            for (e = u.data.length, k = 0; k < e; k += 1)
                                N = (C = u.data[k]) && a.getCleanValue(C.value), null !== N && this._doAlert(a.getCleanValue(C.value))
            },
            _doAlert: function(d) {
                var a = this.linkedItems.chart, n = this.config.alertArr, L = n.length, D, u, e;
                for (u = 0; u < L; u += 1)
                    if (D = n[u], e = D.action && D.action.toLowerCase(), D.minvalue <= d && D.maxvalue >= d) {
                        if ("1" !== D.occuronce ||!D.hasOccurred) {
                            D.hasOccurred=!0;
                            D.state = "1";
                            switch (e) {
                            case "calljs":
                                setTimeout(k.pseudoEval(D.param));
                                break;
                            case "showannotation":
                                a.showAnnotation && a.showAnnotation(D.param)
                            }
                            b.raiseEvent("AlertComplete",
                            {
                                alertValue: d,
                                alertMaxValue: D.maxvalue,
                                alertMinValue: D.minvalue
                            }, a.chartInstance)
                        }
                    } else 
                        "showannotation" === e && "1" === D.state && a.hideAnnotation && a.hideAnnotation(D.param), D.state = "2"
                }
            }
        ])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-realtime", function() {
        var b = this, k = b.window, d = Math.random, a = b.hcLib.pluckNumber, n = function(a, b, d) {
            clearTimeout(d);
            return setTimeout(a, b)
        }, L;
        L = function(D) {
            var u = D.sender, e = u.__state, C, N, J, K, I, G, Z, S, c, F;
            e.dataSetDuringConstruction&&!e.rtStateChanged && void 0 === e.rtPreInit &&
            (u.dataReady() ? (e.rtStateChanged=!0, e.rtPreInit=!0) : e.rtPreInit=!1);
            e.rtStateChanged && (e.rtStateChanged=!1, J = (N = u.jsVars) && N.instanceAPI) && (K = J.config || {}, C = J.jsonData && J.jsonData.chart, J = K && K.chart || {}, I = 1E3 * a(K.updateInterval, K.refreshInterval), G = 1E3 * a(K.clearInterval, 0), Z = K.dataStreamURL, K=!!(K && K.realtimeEnabled && 0 < I && void 0 !== Z && "" !== Z && J), S = e._rtAjaxObj, c = function() {
                u.clearChart && u.clearChart();
                G && (e._toClearChart = setTimeout(c, G))
            }, F = function() {
                var c = Z, a = C && C.datastamp, c = c + (( - 1 === Z.indexOf("?") ?
                "?num=" : "&num=") + d());
                a && (c += "&dataStamp=" + a);
                S.open && S.abort();
                S.get(c);
                e._rtAjaxLatencyStart = new Date
            }, 0 >= I ? (e._toRealtime = clearTimeout(e._toRealtime), S && S.abort()) : 10 > I && (I = 10), e._toClearChart = clearTimeout(e._toClearChart), 0 < G && (10 > G ? G = 10 : e._toClearChart = setTimeout(c, G)), e._rtStaticRefreshMS = I, K && (void 0 === e._rtPaused && (e._rtPaused=!1), e._rtDataUrl = Z, e.lastSetValues = null, S = e._rtAjaxObj || (e._rtAjaxObj = new b.ajax), S.onSuccess = function(c, a, b, f) {
                if (!u.disposed) {
                    a = N.instanceAPI;
                    b = a.feedData;
                    var r = {},
                    d = a.config;
                    e._rtAjaxLatencyStart && (e._rtAjaxLatency = new Date - e._rtAjaxLatencyStart);
                    if (b && d.realtimeEnabled && Z) {
                        a._getPrevData();
                        a.feedData(c, !0, f, e._rtAjaxLatency || 0);
                        c = (r.realtimeDrawingLatency || 0) + (e._rtAjaxLatency || 0);
                        try {
                            k.FC_ChartUpdated && k.FC_ChartUpdated(D.sender.id)
                        } catch (G) {
                            setTimeout(function() {
                                throw G;
                            }, 1)
                        }
                        e._rtPaused || (c >= e._rtStaticRefreshMS && (c = e._rtStaticRefreshMS - 1), e._toRealtime = setTimeout(F, e._rtStaticRefreshMS - c))
                    } else 
                        e._toRealtime = clearTimeout(e._toRealtime)
                }
            }, S.onError = function(c,
            a, d, f) {
                e._rtAjaxLatencyStart && (e._rtAjaxLatency = new Date - e._rtAjaxLatencyStart);
                b.raiseEvent("realtimeUpdateError", {
                    source: "XmlHttpRequest",
                    url: f,
                    xmlHttpRequestObject: a.xhr,
                    error: c,
                    httpStatus: a.xhr && a.xhr.status ? a.xhr.status: - 1,
                    networkLatency: e._rtAjaxLatency
                }, D.sender);
                e._toRealtime = u.isActive() ? setTimeout(F, I) : clearTimeout(e._toRealtime)
            }, e._rtPaused || (e._toRealtime = n(F, 0, e._toRealtime))))
        };
        b.addEventListener(["beforeDataUpdate", "beforeRender"], function(a) {
            a = a.sender;
            var b = a.__state;
            a.jsVars && (a.jsVars._rtLastUpdatedData =
            null);
            b._toRealtime && (b._toRealtime = clearTimeout(b._toRealtime));
            b._toClearChart && (b._toClearChart = clearTimeout(b._toClearChart));
            b._rtAjaxLatencyStart = null;
            b._rtAjaxLatency = null
        });
        b.addEventListener(["renderComplete", "dataUpdated"], function(a) {
            var b = a.sender.__state;
            b && (void 0 === b.rtPreInit && (b.rtPreInit=!1), b._rtPaused && delete b._rtPaused, b.rtStateChanged || (b.rtStateChanged=!0, L.apply(this, arguments)))
        });
        b.core.addEventListener("beforeDispose", function(a) {
            a = a.sender.__state;
            a._toRealtime && (a._toRealtime =
            clearTimeout(a._toRealtime));
            a._toClearChart && (a._toClearChart = clearTimeout(a._toClearChart))
        });
        b.core.addEventListener("drawComplete", L)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-widgets", function() {
        function b() {}
        var k = this.hcLib, d = k.extend2, a = k.toPrecision;
        defaultGaugePaletteOptions = d({}, k.defaultGaugePaletteOptions);
        Array.prototype.forEach || (Array.prototype.forEach = function(a, b) {
            var d, u, e, k, N;
            if (null == this)
                throw new TypeError(" this is null or not defined");
            e = Object(this);
            k = e.length>>>
            0;
            if ("function" !== typeof a)
                throw new TypeError(a + " is not a function");
            1 < arguments.length && (d = b);
            for (u = 0; u < k;)
                u in e && (N = e[u], a.call(d, N, u, e)), u++
        });
        b.prototype = {
            numDecimals: function(b) {
                b = a(b, 10);
                b = Math.abs(b);
                b = a(b - Math.floor(b), 10);
                b = String(b).length - 2;
                return 0 > b ? 0 : b
            },
            toRadians: function(a) {
                return a / 180 * Math.PI
            },
            toDegrees: function(a) {
                return a / Math.PI * 180
            },
            flashToStandardAngle: function(a) {
                return - 1 * a
            },
            standardToFlashAngle: function(a) {
                return - 1 * a
            },
            flash180ToStandardAngle: function(a) {
                var b = 360 - (0 > (a%=
                360) ? a + 360 : a);
                return 360 == b ? 0 : b
            },
            getAngularPoint: function(a, b, d, u) {
                u*=Math.PI / 180;
                return {
                    x: a + d * Math.cos(u),
                    y: b - d * Math.sin(u)
                }
            },
            remainderOf: function(a, b) {
                return roundUp(a%b)
            },
            boundAngle: function(a) {
                return 0 <= a ? b.prototype.remainderOf(a, 360) : 360 - b.prototype.remainderOf(Math.abs(a), 360)
            },
            toNearestTwip: function(a) {
                var b = 0 > a?-1 : 1;
                a = mathRound(100 * Math.abs(a));
                var d = Math.floor(a / 5);
                return (2 < Number(String(a - 5 * d)) ? 5 * d + 5 : 5 * d) / 100 * b
            },
            roundUp: function(a, b) {
                var d = mathPow(10, void 0 === b ? 2 : b);
                a = mathRound(Number(String(a *
                d)));
                return a/=d
            }
        };
        b.prototype.constructor = b;
        k.MathExt = b
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-funnelpyramidbase", function() {
        var b = this, k = b.hcLib, d = k.pluck, a = k.pluckNumber, n = k.BLANKSTRING, L = k.preDefStr, D = L.showHoverEffectStr, u = k.graphics.convertColor, e = k.extend2, C = k.parseUnsafeString, N = k.graphics.getLightColor, J = k.COMMASTRING, K = k.ZEROSTRING, I = k.getValidValue, G = k.parseTooltext, Z = k.setLineHeight, S = Math, c = S.round, F = S.ceil, g = S.max, H = S.min, B = S.pow, f = S.sqrt, r = k.COMMASPACE, y = {},
        W = k.graphics.getDarkColor, p = L.colors.c000000, ia = L.configStr, v = L.animationObjStr, l = L.POSITION_START, ra = L.POSITION_MIDDLE, L = b.window, L = /msie/i.test(L.navigator.userAgent)&&!L.opera, Da = k.regex.hexcode, m = "rgba(192,192,192," + (L ? .002 : 1E-6) + ")", h = k.regex.dropHash, w = k.HASHSTRING, q = k.toRaphaelColor, z = k.plotEventHandler, A = k.regex.startsRGBA, x = function(c) {
            var a = [], t;
            (function(c) {
                (t = /rgba\(\s*([0-9]{1,3})\s*,\s*([0-9]{1,3})\s*,\s*([0-9]{1,3})\s*,\s*([0-9]?(?:\.[0-9]+)?)\s*\)/.exec(c)) ? a = [parseInt(t[1], 10), parseInt(t[2],
                10), parseInt(t[3], 10), parseFloat(t[4])] : (t = /#([a-fA-F0-9]{2})([a-fA-F0-9]{2})([a-fA-F0-9]{2})/.exec(c)) && (a = [parseInt(t[1], 16), parseInt(t[2], 16), parseInt(t[3], 16), 1])
            })(c);
            return {
                get: function(t) {
                    return a&&!isNaN(a[0]) ? "rgb" === t ? "rgb(" + a[0] + "," + a[1] + "," + a[2] + ")" : "hex" === t ? "#" + (p + (a[0]<<16 | a[1]<<8 | a[2]).toString(16)).slice( - 6) : "a" === t ? a[3] : "rgba(" + a.join(",") + ")" : c
                },
                brighten: function(c) {
                    if (!isNaN(c) && 0 !== c) {
                        var t;
                        for (t = 0; 3 > t; t++)
                            a[t] += parseInt(255 * c, 10), 0 > a[t] && (a[t] = 0), 255 < a[t] && (a[t] = 255)
                    }
                    return this
                },
                setOpacity: function(c) {
                    a[3] = c;
                    return this
                }
            }
        };
        E.register("component", ["dataset", "FunnelPyramidBase", {
            type: "funnelpyramidbase",
            pIndex: 2,
            customConfigFn: "_createDatasets",
            init: function(c) {
                var a = this.utils(this).invokeHookFns, t = this.postInitHook;
                if (!c)
                    return !1;
                this.JSONData = c;
                this.components = {};
                this.conf = {};
                this.graphics = {};
                a(t);
                this.configure()
            },
            removeData: function() {},
            _configure: function() {
                var c = this.chart, q = c.components.colorManager, t = c.config, h = t.style, p = this.components, b = this.conf || {}, g = this.JSONData.data ||
                [], c = c.jsonData ? c.jsonData.chart: {}, l = this.utils(this), f = l.invokeHookFns, l = l.copyProperties, m = k.setLineHeight, t = t.canvasHeight, x = this.configureSpecifics, v = this.preDrawingHook, z = q.getColor("baseFontColor");
                l(c, b, [["showlabels", "showLabels", a, 1], ["showvalues", "showValues", a, 1], ["plottooltext", "toolText", d, n], ["enableslicing", "enableSlicing", a, 1], ["plotfillalpha", "plotFillAlpha", a, 100], ["showplotborder", "showPlotBorder", a, 0], ["plotborderalpha", "plotBorderAlpha", a, void 0], ["plotbordercolor", "plotBorderColor",
                d, void 0], ["plotborderthickness", "plotBorderThickness", a, 1], ["showshadow", "showShadow", a, 1], ["showhovereffect", D, a, 0], ["hovercapsepchar", "hoverCapSepChar", d, r], ["tooltipsepchar", "tooltipSepChar", d, "$hoverCapSepChar"], ["labelsepchar", "labelSepChar", d, "$tooltipSepChar"], ["showpercentintooltip", "showPercentInToolTip", a, 1], ["showpercentvalues", "showPercentValues", a, 0], [n, "slicingDistance", a, .1 * t], ["slicingdistance", "slicingHeight", a, "$slicingDistance", function(c) {
                    c.slicingDistance = c.slicingHeight > 2 * c.slicingDistance ?
                    0 : c.slicingHeight
                }
                ], [n, "blankSpace", a, 3], ["labeldistance", "labelDistance", a, 50], ["issliced", "isSliced", a, 0], ["is2d", "is2d", a, 0], [n, "blankSpace", a, 3], ["showlabelsatcenter", "showLabelsAtCenter", a, 0], ["smartlinethickness", "connectorWidth", a, 1], ["smartlinealpha", "connectorAlpha", a, 100], ["smartlinecolor", "rawSmartLineColorCode", d, function() {
                    return q.getColor("baseFontColor")
                }
                ], ["labelalpha", "labelAlpha", a, 100], ["basefont", "baseFont", d, "Verdana,sans"], ["basefontsize", "baseFontSize", a, 10], ["basefontcolor",
                "baseFontColor", d, z], ["labelfontcolor", "labelFontColor", d, "$baseFontColor"], ["showtooltip", "showTooltip", a, 1], ["percentofprevious", "percentOfPrevious", a, 0], ["animationduration", "animationDuration", a, 1, function(c) {
                    c.animationDuration*=1E3
                }
                ]]);
                b.connectorColor = u(b.rawSmartLineColorCode, b.connectorAlpha);
                m(h);
                h.nLineHeight = h.lineHeight.match(/^\d+/)[0];
                l(h, b, [[n, "lineHeight", a, b.baseFontSize]]);
                delete h.nLineHeight;
                f(x);
                b.datalabelDisabled = b.showLabels || b.showValues?!1 : !0;
                p.data = this.getNormalizeDataSet(g);
                f(v)
            },
            _checkValidData: function(c) {
                var a = this.chart;
                return c && c.length?!0 : (a.setChartMessage(), !1)
            },
            addLegend: function() {
                var c = this.chart, q = c.jsonData.chart, t, h = this.JSONData.data, b = c.components.legend;
                b.emptyItems();
                for (c = 0; c < h.length; c += 1)
                    t = h[c], t.pseudoPoint || (t.legendItemId = b.addItems(this, void 0, {
                        type : this.type, label : t.label, index : c, enabled : a(q.includeinlegend, 1), legendItemId : t.legendItemId
                    }))
            },
            getNormalizeDataSet: function(c, q) {
                var t = this.getPointInContext(), h = this.chart, b = this.conf, p = h.components,
                g = p.numberFormatter, l = this.chart.linkedItems.smartLabel, p = p.colorManager, f, m = [], x, r, v, z = 0, w, A, y, H, ia, F = 0, B, W, ra, D, S, P, Da = b.showPercentValues, Z = b.labelSepChar, L = h.jsonData.chart, E = b.isSliced, ja, ka, Fa, Ia, ea = {
                    apply: b.showShadow,
                    opacity: 1
                }, va, ga, Ha = b.plotBorderThickness;
                x = b.dataConnectorStyle = {};
                var Ca = h.config.PLOT_COLOR_INDEX_START, Ja, Na, xa, ca;
                e(va = b.style = {}, h.config.style);
                va.borderDash = "none";
                va.borderPadding = 2;
                va.borderRadius = 0;
                va.borderThickness = 1;
                va.color = u(b.labelFontColor, b.labelAlpha);
                va.fontFamily =
                b.baseFont;
                va.fontSize = b.baseFontSize + "px";
                va.fontStyle = "normal";
                va.fontWeight = "normal";
                x.connectorWidth = b.connectorWidth;
                x.connectorColor = b.connectorColor;
                Ja = this.datasetCalculations(c);
                r = Ja.hasValidPoint;
                x = Ja.refreshedData;
                z = Ja.sumValue;
                v = Ja.highestValue;
                this._chartLevelAttr = k.parsexAxisStyles({}, {}, L, va);
                if (r)
                    for (b.sumValue = z, r = g.dataLabels(z), w = x.length, l.useEllipsesOnOverflow(h.config.useEllipsesWhenOverflow), h = 0; h < w; h += 1) {
                        f = x[h];
                        Na = f.legendItemId;
                        A = f.cleanValue;
                        y = h ? x[h - 1].value : A;
                        H = C(d(f.label,
                        f.name, n));
                        ia = l.getOriSize(H);
                        ka = f.alpha || b.plotFillAlpha;
                        xa = d(f.color, p.getPlotColor(Ca++));
                        ca = u(xa);
                        ja = u(xa, ka);
                        Fa = d(f.bordercolor, b.plotBorderColor, N(xa, 25)).split(J)[0];
                        Ia = b.showPlotBorder ? d(f.borderalpha, b.plotBorderAlpha, "80") : K;
                        ea.opacity = Math.max(ka, Ia) / 100;
                        if (B = a(f.issliced, E))
                            F += 1, b.preSliced = B;
                            Ja.prevPerValReq && (z = y);
                            W = g.percentValue(A / z * 100);
                            b.datalabelDisabled || (ra = g.dataLabels(A) || n, D = 1 === b.showLabels ? H : n, S = 1 === a(f.showvalue, b.showValues) ? 1 === Da ? W : ra : n, P = I(C(f.displayvalue)), ga = d(P,
                            H + Z + (Da ? W : ra), n), S = P ? P : S !== n && D !== n ? D + Z + S : d(D, S) || n);
                            D = I(C(d(f.tooltext, b.toolText)));
                            void 0 !== D ? (y = {
                                formatedVal: ra,
                                name: H,
                                pValue: W,
                                sum: r,
                                sumValue: r,
                                dataValue: A,
                                prevValue: y,
                                highestValue: v
                            }, D = G(D, [1, 2, 3, 7, 14, 24, 25, 37], this.getTooltipMacroStub(y), f, L)) : (D = 1 === b.showPercentInToolTip ? W : ra, D = H !== n ? H + b.tooltipSepChar + D : D);
                            y = this.pointHoverOptions(f, {
                                color: xa,
                                alpha: ka,
                                borderColor: Fa,
                                borderAlpha: Ia,
                                borderWidth: Ha
                            });
                            W = S;
                            P = ga;
                            var Ga = k.parsexAxisStyles(f, {}, L, va, ja), Oa;
                            Oa = f;
                            var Ka = {
                                labelfont: "fontFamily",
                                labelfontcolor: "color",
                                labelfontsize: "fontSize",
                                labelfontbold: "fontWeight",
                                labelfontitalic: "fontStyle"
                            }, Ea = void 0, La = void 0;
                            for (La in Ka)
                                La in Oa && (Ea = Ea || {}, Ea[Ka[La]] = Oa[La]);
                                Ea && (Ea.fontWeight && (Ea.fontWeight = a(Ea.fontWeight) ? "bold" : "normal"), Ea.fontStyle && (Ea.fontStyle = a(Ea.fontStyle) ? "italic" : "normal"));
                                Oa = Ea;
                                f = {
                                    displayValue: W,
                                    displayValueArgs: P,
                                    style: Ga,
                                    appliedStyle: Oa,
                                    name: H,
                                    categoryLabel: H,
                                    rawColor: xa,
                                    rawAlpha: ka,
                                    toolText: D,
                                    legendCosmetics: void 0,
                                    legendItemId: Na,
                                    showInLegend: void 0,
                                    y: A,
                                    shadow: ea,
                                    smartTextObj: ia,
                                    legendColor: ca,
                                    color: ja,
                                    alpha: ka,
                                    borderColor: u(Fa,
                                    Ia),
                                    borderWidth: Ha,
                                    link: I(f.link),
                                    isSliced: B,
                                    doNotSlice: !b.enableSlicing,
                                    hoverEffects: y.enabled && y.options,
                                    rolloverProperties: y.enabled && y.rolloverOptions
                                };
                                m.push(new t(f))
                    }
                b.noOFSlicedElement = F;
                return m
            },
            datasetCalculations: function(c) {
                var a = this.chart.components.numberFormatter, t, q, b, h, p = {
                    refreshedData: []
                };
                p.sumValue = p.countPoint = 0;
                p.highestValue = Number.NEGATIVE_INFINITY;
                t = 0;
                for (q = c.length; t < q; t++)
                    b = c[t], b.vline || (b.cleanValue = h = Math.abs(a.getCleanValue(b.value,
                    !0)), null !== h && (p.hasValidPoint=!0, p.highestValue = p.highestValue || h, p.refreshedData.push(b), p.sumValue += h, p.countPoint += 1, p.highestValue = Math.max(p.highestValue, p.itemValue)));
                return p
            },
            pointHoverOptions: function(c, b) {
                var t = this.chart, q = a(c.showhovereffect, this.conf.showHoverEffect), h = {
                    enabled: q
                }, p = {}, f, t = t.jsonData ? t.jsonData.chart: {};
                q || (q = h.enabled = void 0 !== d(c.hovercolor, t.plotfillhovercolor, c.hoveralpha, t.plotfillhoveralpha, c.borderhovercolor, t.plotborderhovercolor, c.borderhoverthickness, t.plotborderhoverthickness,
                c.borderhoveralpha, t.plotborderhoveralpha));
                if (q) {
                    h.highlight = a(c.highlightonhover, t.highlightonhover);
                    h.color = d(c.hovercolor, t.plotfillhovercolor);
                    h.alpha = d(c.hoveralpha, t.plotfillhoveralpha, b.alpha);
                    h.borderColor = d(c.borderhovercolor, t.plotborderhovercolor, b.borderColor);
                    h.borderThickness = a(c.borderhoverthickness, t.plotborderhoverthickness, b.borderWidth);
                    h.borderAlpha = d(c.borderhoveralpha, t.plotborderhoveralpha, b.borderAlpha);
                    0 !== h.highlight && void 0 === h.color && (h.highlight = 1);
                    h.color = d(h.color,
                    b.color).replace(/,+?$/, n);
                    if (1 === h.highlight) {
                        p = h.color.split(/\s{0,},\s{0,}/);
                        t = p.length;
                        for (f = 0; f < t; f += 1)
                            p[f] = N(p[f], 70);
                        h.color = p.join(",")
                    }
                    p = {
                        color: h.color,
                        alpha: + h.alpha,
                        borderColor: u(h.borderColor, h.borderAlpha),
                        borderWidth: h.borderThickness
                    }
                }
                return {
                    enabled: q,
                    options: h,
                    rolloverOptions: p
                }
            },
            getTooltipMacroStub: function(c) {
                return {
                    formattedValue: c.formatedVal,
                    label: c.name,
                    percentValue: c.pValue,
                    sum: c.sum,
                    unformattedSum: c.sumValue
                }
            },
            preDrawingSpaceManagement: function() {
                var c = this.getPointInContext(),
                h = this.chart, t = h.config, b = h.components, q = b.caption.config.height || 0, p = b.subCaption.config.height || 0, f = this.conf, l = f.showTooltip, x = f.slicingDistance, m;
                m = t.height - (t.marginTop + t.marginBottom);
                var r = t.width - (t.marginRight + t.marginLeft), v = this.components.data, z, w;
                z = f.blankSpace;
                var d = this.LABEL_PLACEMENT_ITERATOR_INDEX_START, c = c.upperRadiusFactor, A, y, G, H, ia, B, u = h.linkedItems.smartLabel, W, e, ra, n, K, k, D;
                K = 0;
                var I, C = 0;
                W = this.utils(this).invokeHookFns;
                K = this.prePointProcessingHookFn;
                var S = b.legend, Da = 0;
                "right" ===
                S.config.legendPos && (Da = S.config.width + 10);
                t.oriCanvasLeft = h.config.canvasLeft;
                t.oriBottomSpace = h.config.marginBottom;
                t.oriTopSpace = h.config.marginTop;
                b = f._tempSnap = {
                    top3DSpace: 0,
                    bottom3DSpace: 0,
                    topLabelSpace: 0,
                    rightLabelSpace: 0
                };
                m = Math.min(2 * (m - x), r);
                t.marginTop += x / 2;
                t.marginBottom += x / 2;
                x = v.length;
                z = f.labelDistance + z;
                w = f.showLabelsAtCenter;
                A = Math.min(m, .3 * r);
                y = r - A;
                r -= Da;
                G = r - A - z;
                H = 0;
                ia = v[0] && v[0].y ? v[0].y : 1;
                B = .8 / ia;
                W(K, [v]);
                W = f.totalValue || 0;
                for (u.useEllipsesOnOverflow(t.useEllipsesWhenOverflow); d <
                x; d += 1)
                    e = v[d], e.legendItemId && S.configureItems(e.legendItemId, {
                        configuration: {
                            fillColor: e.legendColor
                        }
                    }), K = e.style, Z(K), ra = a(F(parseFloat(K.lineHeight) + K.borderPadding + K.borderThickness + 5), 10), u.setStyle(K), K = e.y, w ? u.getSmartText(e.displayValue, r, ra) : (K = e.getModifiedCurrentValue && e.getModifiedCurrentValue(W) || K, K = e.getRatioK(K, B, W, ia), k = A * K, D = G + (A - k) / 2, n = u.getSmartText(e.displayValue, D, ra), e.displayValue = n.text, l && n.tooltext && (e.originalText = n.tooltext), H = Math.max(H, n.width), 0 < y && (n = 0 < n.width ? D - n.width :
                    D + z, K = 1 / (K + 1) * (k + 2 * n + A), y = Math.min(y, K - A)), W += void 0 === f.offsetVal ? e.y : "function" === typeof f.offsetVal ? f.offsetVal(d) : f.offsetVal);
                "right" === S.config.legendPos ? (h.isLegendRight=!0, t.marginRight += Da) : h.isLegendRight=!1;
                e && (I = e.getLowestRadiusFactor(ia));
                h = A + y;
                h > m && (h = m);
                W = void 0 === f.offsetVal ? 0 : "function" === typeof f.offsetVal ? f.offsetVal() : f.offsetVal;
                if (!w)
                    for (d = this.LABEL_PLACEMENT_ITERATOR_INDEX_START, x = v.length; d < x; d += 1)
                        e = v[d], K = e.y, K = e.getModifiedCurrentValue && e.getModifiedCurrentValue(W) || K,
                        K = e.getRatioK(K, B, W, ia), k = h * K, D = G + (A - k) / 2 - Da, n = u.getSmartText(e.displayValue, D, ra), C = g(C, .5 * k + n.width + z), W += void 0 === f.offsetVal ? e.y : "function" === typeof f.offsetVal ? f.offsetVal() : f.offsetVal;
                0 < H ? (b.rightLabelSpace = r - h, v = C - .5 * t.canvasWidth, 0 < v && (t.marginRight += v, t.marginLeft -= v), t.marginRight += .5 * b.rightLabelSpace, t.marginLeft += .5 * b.rightLabelSpace) : z = 0;
                f.labelDistance = f.connectorWidth = z;
                (w ||!H) && m < r && (t.marginLeft += .5 * (r - m - z), t.marginRight += .5 * (r - m - z));
                f.is2d || (t.marginTop += b.top3DSpace = h * f.yScale *
                c / 2, t.marginBottom += b.bottom3DSpace = h * f.yScale * I / 2);
                t.gaugeStartX = t.marginLeft;
                t.gaugeStartY = t.marginTop + (q + p + 5);
                t.gaugeEndX = h + t.marginLeft;
                t.gaugeEndY = t.canvasHeight + t.marginTop;
                t.gaugeCenterX = t.gaugeStartX + (t.gaugeEndX - t.gaugeStartX) / 2 - t.marginLeft / 2;
                t.gaugeCenterY = t.gaugeStartY + (t.gaugeEndY - t.gaugeStartY) / 2;
                t.plotSemiWidth = (t.canvasWidth - b.rightLabelSpace) / 2;
                t.canvasCenterX = t.oriCanvasLeft + h / 2
            },
            hide: function(c) {
                var h, a, b;
                if (c && 0 !== c.length)
                    for (h = 0, a = c.length; h < a; h++)
                        b = c[h], b.connector && b.connector.hide(),
                        b.dataLabel && b.dataLabel.hide(), b.graphic && b.graphic.hide(), b.trackerObj && b.trackerObj.hide()
            },
            animateElements: function(c, h, a, b, q) {
                function p() {
                    z || (q(), z=!0)
                }
                function f(c, h) {
                    var a = (h || {}).alpha, a = void 0 === a ? b.post : {
                        opacity: a
                    };
                    c && (c.attr(b.pre), c.animateWith(m, x, a, l, r, p))
                }
                var g = this.chart.get(ia, v), l = g.duration, m = g.dummyObj, x = g.animObj, r = g.animType, z=!1, d, w, A, y, G;
                q = q || function() {};
                a = a || [];
                d = 0;
                for (w = c.length; d < w; d++)
                    if (A = c[d])
                        if (a.length)
                            for (y = 0, G = a.length; y < G; y++)
                                g = c[d][h][a[y]], f(g, A.point);
                        else 
                            g =
                            A, f(g[h], A.point)
            },
            drawIndividualDataLabel: function(c, h) {
                var b = this.conf, q = this.components.data, p = c.displayValue, f = c.plot, g = c.labelY, l = c.labelX, m = c.style || {}, x = a(parseInt(m.fontSize, 10), b.baseFontSize), r = b.lineHeight, v = .3 * x, z = .3 * r, d, w = b.connectorEndSwitchHistoryY, A = b.labelDistance, y = b.blankSpace, x = b.streamLinedData;
                b.showLabelsAtCenter ? (q = 0 === h && x ? g - z + (q[1].plot.distanceAvailed || 0) : g - z + (f.distanceAvailed || 0), p !== n ? f.dataLabel.transform("t" + l + "," + q).show() : f.dataLabel && f.dataLabel.hide()) : (z = g - v - c.distributionFactor *
                r, v = g - v, d = w[c.alignmentSwitch], void 0 !== b.lastplotY && void 0 !== d && d - v < r && (g = v = d - r), c.displayValue && (w[c.alignmentSwitch] = v), b.lastplotY = c.plotY, b.labelAlignment === b.alignmentType.alternate ? c.alignmentSwitch ? (r = l + y + c.virtualWidth, b = r + A + c.distributionFactor * b.globalMinXShift) : (r = l - y, b = r - (A - (c.lOverflow || 0)) - c.distributionFactor * b.globalMinXShift) : (r = l - y, b = r - (A - (c.lOverflow || 0)) - c.distributionFactor * b.globalMinXShift), "undefined" === typeof p || p === n || 0 === h && x ? f.connector && f.connector.hide() : (b = ["M", b, z,
                "L", r, v], f.connector.attr({
                    path: b,
                    "shape-rendering": z === v && 1 > v ? "crisp": n
                }).show()), q = 0 === h && x ? g + (q[1].plot.dy || 0) : v + (f.dy || 0), p !== n ? f.dataLabel.transform("t" + l + J + q).show() : f.dataLabel && f.dataLabel.hide());
                f.dataLabel.attr({
                    "text-bound": [m.backgroundColor, m.borderColor, m.borderThickness, m.borderPadding, m.borderRadius, m.borderDash]
                })
            },
            drawAllLabels: function() {
                var c = this.chart, b = c.graphics, h = this.graphics.plotItems, a = this.labelDrawingConfig, q = b.datalabelsGroup, p = ["fontFamily", "fontSize", "fontWeight",
                "fontStyle"], f = {}, c = c.components.paper, g = this.conf, l, m, x, r;
                r = 0;
                for (m = p.length; r < m; r++)
                    x = p[r], x in this._chartLevelAttr && (f[x] = this._chartLevelAttr[x]);
                b.datalabelsGroup.css(f);
                for (r = a.length - 1; - 1 < r; r--)
                    b = a[r], f = b.point, m=!!f.link, x = f.y, b.args && b.css && (b.args.fill = b.css.color || b.css.fill), null !== x && void 0 !== x && f.shapeArgs ? ((l = h[r]) && l.dataLabel ? (l.dataLabel.removeCSS(p), l.dataLabel.attr(b.args).css(b.css)) : (f.plot.dataLabel = l.dataLabel = c.text(b.args, b.css, q), g.showLabelsAtCenter && 0 === r && g.streamLinedData ||
                (l.connector = c.path(q))), l.dataLabel.tooltip(f.originalText), l.connector && l.connector.attr({
                    "stroke-width": g.dataConnectorStyle.connectorWidth,
                    stroke: g.dataConnectorStyle.connectorColor,
                    ishot: !0,
                    cursor: m ? "pointer": n,
                    transform: b.transform
                }), m = b.actions, !f.doNotSlice && l.dataLabel.click(m.click, b.context), l.dataLabel.hover(m.hover[0], m.hover[1]), l.dataLabel.attr({
                    transform: b.transform
                })) : h[r] ? (f.plot = h[r], m = f.plot.dataLabel, m.removeCSS(p), m && m.attr(b.args).css(b.css)) : f.plot = h[r] = {
                    dataLabel: c.text(b.args,
                    b.css, q)
                }, this.drawIndividualDataLabel(f, r)
            },
            drawAllTrackers: function() {
                var c = this.trackerArgs, b, h;
                b = 0;
                for (h = c.length; b < h; b++)
                    this.drawTracker(c[b])
            },
            drawTracker: function(c) {
                var b = this.chart, h = b.components.paper, b = b.graphics.trackerGroup, a, q, f =+ new Date, p, g;
                if (p = c.plot)
                    q = p.trackerObj, p.graphic && (a = p.graphic.Shapeargs.silhuette, g = {
                        link: c.link,
                        value: c.y,
                        displayValue: c.displayValueArgs,
                        categoryLabel: c.categoryLabel,
                        dataIndex: p.index || n,
                        toolText: c.toolText
                    }, c.datasetIndex = p.index, q ? q.attr({
                        path: a,
                        isTracker: f,
                        fill: m,
                        stroke: "none",
                        transform: "t0," + (p._startTranslateY || 0),
                        ishot: !0,
                        cursor: p.cursor
                    }) : q = p.trackerObj = h.path(a, b).attr({
                        isTracker: f,
                        fill: m,
                        stroke: "none",
                        transform: "t0," + (p._startTranslateY || 0),
                        ishot: !0,
                        cursor: p.cursor
                    }), q.data("eventArgs", g), q.show())
            },
            calculatePositionCoordinate: function(c, h) {
                var a = this.conf, q = a.maxValue, p = a.is2d, g = a.x, m = this.graphics.plotItems || [], x = this.chart, r = x.config, v = r.canvasTop, z = a.unitHeight, d = a.drawingRadius, A = a.labelDistance, w = a.showLabelsAtCenter, y = a.isHollow, G = .3 * r.style.fontSize,
                H = a.yScale, ia = a.blankSpace, W = a.lastRadius, F = x.linkedItems.smartLabel, B, u = 0, e, K, n = c.length - 1, k=!1, D = 0, I, C, S = a.lineHeight, Da = .8 / r.effCanvasHeight, Z = x.config.width - 2, J = a.streamLinedData;
                B = {
                    flag: !1,
                    point: void 0,
                    sLabel: void 0,
                    setAll: function(c,
                    b,
                    a) {
                        this.flag = c;
                        this.point = b;
                        this.sLabel = a
                    }
                };
                var N = {
                    point: void 0,
                    sLabel: void 0,
                    set: function(c,
                    b) {
                        return function(a,
                        h) {
                            var q,
                            p;
                            a.dontPlot || (this.point && this.sLabel ? (q = c(this.point,
                            this.sLabel),
                            p = c(a,
                            h),
                            b(q,
                            p) && (this.point = a,
                            this.sLabel = h)): (this.point = a,
                            this.sLabel =
                            h))
                        }
                    }
                }, L = {}, E = {}, ga = {}, Ha = {}, Ca = a.slicingGapPosition = {};
                b.extend(L, B);
                b.extend(E, B);
                L.setAll = function(c, b, a) {
                    var h = this.point, q = this.sLabel;
                    this.flag = c;
                    h && q ? (c = h.labelX - (q.oriTextWidth - q.width), h = b.labelX - (a.oriTextWidth - a.width), c > h && (this.point = b, this.sLabel = a)): (this.point = b, this.sLabel = a)
                };
                E.setAll = function(c, b, a) {
                    var h = this.point, q = this.sLabel;
                    this.flag = c;
                    h && q ? (c = h.labelX + q.oriTextWidth, h = b.labelX + a.oriTextWidth, c < h && (this.point = b, this.sLabel = a)): (this.point = b, this.sLabel = a)
                };
                b.extend(ga, N);
                b.extend(Ha, N);
                ga.set = function() {
                    return N.set.apply(ga, [function(c) {
                        return c.labelX
                    }, function(c, b) {
                        return c > b?!0: !1
                    }
                    ])
                }();
                Ha.set = function() {
                    return N.set.apply(Ha, [function(c, b) {
                        return c.labelX + b.oriTextWidth
                    }, function(c, b) {
                        return c < b?!0: !1
                    }
                    ])
                }(); a.noOfGap = 0; F.useEllipsesOnOverflow(r.useEllipsesWhenOverflow);
                r = 0;
                for (B = c.length; r < B; r++)
                    if (e = c[r])
                        e.x = r, m[r] && (e.isSliced = m[r].sliced||!!e.isSliced||!!a.isSliced), r ? (h && (k=!k), e.isSliced && (C = e.x, 1 < C&&!Ca[C] && (Ca[C]=!0, a.noOfGap += 1), C < n && (Ca[C + 1]=!0, a.noOfGap +=
                        1)), J ? (C = 1 == a.useSameSlantAngle ? q ? d * e.y / q : d : q ? d * f(e.y / q) : d, K = z * (c[r - 1].y - e.y) || 1) : (u += K = z * c[r].y, C = d * (1 - u * Da)), e.shapeArgs = {
                            x: g,
                            y: v,
                            R1: W,
                            R2: C,
                            h: K || 1,
                            r3dFactor: H,
                            isHollow: y,
                            gStr: "point",
                            is2D: p,
                            renderer: x.components.paper,
                            isFunnel: !0
                        }, F.setStyle(e.style), e.oriText = e.displayValue, I = I = F.getSmartText(e.displayValue, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY), w ? (e.labelAline = ra, e.labelX = g, e.labelY = (p ? v : v + H * W) + K / 2 + G) : (e.labelAline = l, e.alignmentSwitch = k, e.distributionFactor = e.distributionFactor || 0,
                        k ? (e.labelX = g - (A + C + ia + I.width), e.labelX -= e.distributionFactor * a.globalMinXShift, ga.set(e, I)) : (e.labelX = g + A + C + ia, e.labelX += e.distributionFactor * a.globalMinXShift, Ha.set(e, I)), D = e.distributionFactor * S, e.labelY = v + G + K + D), h && (k && 0 > e.labelX ? (W = e.labelX + I.width, W = F.getSmartText(e.displayValue, W, Number.POSITIVE_INFINITY, !0), e.labelX = 2, e.isLabelTruncated=!0, e.displayValue = W.text, e.virtualWidth = W.maxWidth, L.setAll(!0, e, W)) : !k && e.labelX + I.width > Z && (W = F.getSmartText(e.displayValue, Z - e.labelX, Number.POSITIVE_INFINITY,
                        !0), e.isLabelTruncated=!0, e.displayValue = W.text, e.virtualWidth = W.maxWidth, E.setAll(!0, e, W))), e.pWidth = e.virtualWidth = e.virtualWidth || I.width, v += K, W = C) : (e.oriText = e.displayValue, C = 1 == a.useSameSlantAngle ? q ? d * e.y / q : d : q ? d * f(e.y / q) : d, e.labelWidth > 2 * C&&!h ? (e.labelAline = l, e.labelX = 0) : (e.labelAline = ra, e.labelX = g), K = 2 * A, e.displayValue = F.getSmartText(e.displayValue, 2 * C + K, Number.POSITIVE_INFINITY, !0).text, e.labelY = (p ? v : v - H * W) - G - ia), e.plotX = g, e.plotY = v;
                this.findBestPosition.call(this, c, {
                    lTrimmedInfo: L,
                    rTrimmedInfo: E,
                    lLargestLabel: ga,
                    rLargestLabel: Ha
                })
            }, findBestPosition: function(c, b) {
                var a = this.conf, h = this.chart, q = h.config, p = 0, f = b.lTrimmedInfo, g = b.rTrimmedInfo, l = b.lLargestLabel, r = b.rLargestLabel, m = h.linkedItems.smartLabel, x = 0, v = a.streamLinedData, d = a.blankSpace, a = q.width - a.blankSpace;
                if (!f.flag ||!g.flag) {
                    if (g.flag) {
                        if (!l.point)
                            return;
                        p = g.sLabel;
                        p = p.oriTextWidth - p.width;
                        l = l.point.labelX - d;
                        p =- Math.ceil(Math.min(p, l))
                    } else if (f.flag) {
                        if (!r.point)
                            return;
                        p = f.sLabel;
                        p = p.oriTextWidth - p.width;
                        l = a - (r.point.labelX + r.sLabel.width);
                        p = Math.ceil(Math.min(p, l))
                    }
                    if (p)
                        for (l = 0, r = c.length; l < r; l++)
                            q = c[l], !l && v ? q.labelX += p : (q.alignmentSwitch ? (h = 0 > p ? m.getSmartText(q.oriText, q.pWidth, Number.POSITIVE_INFINITY, !0) : m.getSmartText(q.oriText, q.pWidth + p, Number.POSITIVE_INFINITY, !0), q.isLabelTruncated && (x = h.width - q.pWidth)) : h = 0 < p ? m.getSmartText(q.oriText, q.pWidth, Number.POSITIVE_INFINITY, !0) : m.getSmartText(q.oriText, q.pWidth - p, Number.POSITIVE_INFINITY, !0), q.virtualWidth = h.width, q.displayValue = h.text, q.labelX += p - x, q.shapeArgs && (q.shapeArgs.x +=
                            p), x = 0);
                    else 
                        for (m.useEllipsesOnOverflow(q.useEllipsesWhenOverflow), l = 0, r = c.length; l < r; l++)
                            q = c[l], 0 < (x = q.labelX + q.pWidth - a) && (q.lOverflow = x, q.labelX -= x, h.isLegendRight ? q.displayValue = m.getSmartText(q.oriText, q.pWidth - x, Number.POSITIVE_INFINITY, !0).text : (q.lOverflow = x, q.labelX -= x))
                }
            }, getPlotData: function(c) {
                var b = this.components.data[c], a = this.userData || (this.userData = []), h = "y name color alpha borderColor borderWidth link label displayValue datasetIndex toolText".split(" "), q;
                if (a[c])
                    c = a[c];
                else {
                    c =
                    a[c] = {};
                    for (a = 0; a < h.length; a++)
                        c[q = h[a]] = b[q];
                    c.value = c.y;
                    c.label = c.name;
                    delete c.y;
                    delete c.name
                }
                return c
            }, pyramidFunnelShape: function() {
                var b = {
                    y: !0,
                    R1: !0,
                    R2: !0,
                    h: !0,
                    r3dFactor: !0,
                    color: !0,
                    opacity: !0,
                    fill: !0,
                    stroke: !0,
                    strokeColor: !0,
                    strokeAlpha: !0,
                    "stroke-width": !0
                }, p = function(b, a, h, q, p, g, l, r, m, x, t) {
                    "object" === typeof b && (a = b.y, h = b.R1, q = b.R2, p = b.h, g = b.r3dFactor, l = b.is2D, x = b.isHollow, m = b.isFunnel, b = b.x);
                    r = b - h;
                    t = b + h;
                    var v = b - q, d = b + q, z = a + p, A, w;
                    if (l)
                        A = {
                            silhuette: ["M", r, a, "L", t, a, d, z, v, z, "Z"]
                        }, m || (b = Math.round(b),
                        A.lighterHalf = ["M", r, a, "L", b, a, b, z, v, z, "Z"], A.darkerHalf = ["M", b, a, "L", t, a, d, z, b, z, "Z"]);
                    else if (m) {
                        v = b;
                        d = a;
                        a = h || .01;
                        t = q || .01;
                        b = x;
                        r = a * g;
                        g*=t;
                        p = d + p;
                        z = B(t, 2) - B(a, 2);
                        h =- 2 * (B(t, 2) * d - B(a, 2) * p);
                        q = B(a * g, 2) + B(t * d, 2) - B(t * r, 2) - B(a * p, 2);
                        x = f(B(h, 2) - 4 * z * q);
                        q = ( - h + x) / (2 * z);
                        z = ( - h - x) / (2 * z);
                        q < p && q > d ? w = z : z < p && z > d && (w = q);
                        q = f((B(w - d, 2) - B(r, 2)) / B(a, 2));
                        h =- q;
                        z = {
                            x: c(B(a, 2) * q / (w - d) * 100) / 100,
                            y: c(100 * (B(r, 2) / (w - d) + d)) / 100
                        };
                        q = {
                            x: c(B(t, 2) * q / (w - p) * 100) / 100,
                            y: c(100 * (B(g, 2) / (w - p) + p)) / 100
                        };
                        x = {
                            x: c(B(a, 2) * h / (w - d) * 100) / 100,
                            y: c(100 * (B(r,
                            2) / (w - d) + d)) / 100
                        };
                        w = {
                            x: c(B(t, 2) * h / (w - p) * 100) / 100,
                            y: c(100 * (B(g, 2) / (w - p) + p)) / 100
                        };
                        z = {
                            topLeft: x,
                            bottomLeft: w,
                            topRight: z,
                            bottomRight: q
                        };
                        for (A in z)
                            if (isNaN(z[A].x) || isNaN(z[A].y))
                                z[A].x = "topLeft" === A || "bottomLeft" === A?-a : a, z[A].y = "bottomRight" === A || "bottomLeft" === A ? p : d;
                        p = z.topLeft;
                        h = z.bottomLeft;
                        A = v + p.x;
                        w = v + z.topRight.x;
                        d = v + h.x;
                        v += z.bottomRight.x;
                        p = p.y;
                        h = h.y;
                        z = ["A", a, r, 0, 0, 0, w, p];
                        q = ["A", a, r, 0, 1, 1, w, p];
                        x = ["A", t, g, 0, 0, 1, d, h];
                        t = ["A", t, g, 0, 1, 0, d, h];
                        t = {
                            front: ["M", A, p].concat(z, ["L", v, h], x, ["Z"]),
                            back: ["M",
                            A, p].concat(q, ["L", v, h], t, ["Z"]),
                            silhuette: ["M", A, p].concat(q, ["L", v, h], x, ["Z"])
                        };
                        b || (t.top = ["M", A, p].concat(z, ["L", w, p], ["A", a, r, 0, 1, 0, A, p], ["Z"]));
                        A = t
                    } else 
                        A = h * g, w = q * g, p = H(5, h), h = H(2, 2 * A), q = H(2, h), g = q / g, A = {
                            top: ["M", r, a, "L", b, a + A, t, a, b, a - A, "Z"],
                            front: ["M", r, a, "L", b, a + A, t, a, d, z, b, z + w, v, z, "Z"],
                            topLight: ["M", r, a + .5, "L", b, a + A + .5, b, a + A - h, r + g, a, "Z"],
                            topLight1: ["M", t, a + .5, "L", b, a + A + .5, b, a + A - q, t - g, a, "Z"],
                            silhuette: ["M", r, a, "L", b, a - A, t, a, d, z, b, z + w, v, z, "Z"],
                            centerLight: ["M", b, a + A, "L", b, z + w, b - 5, z + w, b - p, a + A,
                            "Z"],
                            centerLight1: ["M", b, a + A, "L", b, z + w, b + 5, z + w, b + p, a + A, "Z"]
                        };
                    return A
                }, g = function(c, f) {
                    var g, l, r = this, m, t, z=!1, v=!1, d = this._3dAttr, y;
                    "string" === typeof c && void 0 !== f && null !== f && (g = c, c = {}, c[g] = f);
                    if ("string" === typeof c)
                        r = b[c] ? this._3dAttr[c] : this._attr(c);
                    else {
                        for (g in c)
                            l = c[g], b[g] ? (d[g] = l, "fill" === g ? (l && l.linearGradient && l.stops && l.stops[0] && (l = l.stops[0][1]), A.test(l) ? (t = new x(l), m = t.get("hex"), t = 100 * t.get("a")) : l && l.FCcolor ? (m = l.FCcolor.color.split(J)[0], t = l.FCcolor.opacity.split(J)[0]) : Da.test(l) &&
                            (m = l.replace(h, w), t = a(d.opacity, 100)), d.color = m, d.opacity = t, v=!0) : "color" === g || "opacity" === g ? (d.fill = q(u(d.color, a(d.opacity, 100))), v=!0) : "stroke" === g || "strokeColor" === g || "strokeAlpha" === g ? d.is2D && ("stroke" === g ? (l && l.linearGradient && l.stops && l.stops[0] && (l = l.stops[0][1]), A.test(l) ? (t = new x(l), m = t.get("hex"), t = 100 * t.get("a")) : l && l.FCcolor ? (m = l.FCcolor.color.split(J)[0], t = l.FCcolor.opacity.split(J)[0]) : Da.test(l) && (m = l.replace(h, w), t = a(d.opacity, 100)), d.strokeColor = m, d.strokeAlpha = t) : d.stroke = u(d.strokeColor,
                            a(d.strokeAlpha, 100)), d.isFunnel ? this.funnel2D.attr("stroke", d.stroke) : this.borderElement.attr("stroke", d.stroke)) : "stroke-width" === g ? d.is2D && (d.isFunnel ? this.funnel2D.attr(g, l) : this.borderElement.attr(g, l)) : z=!0) : this._attr(g, l);
                        d.is2D ? (z && (m = p(d.x, d.y, d.R1, d.R2, d.h, d.r3dFactor, d.is2D), r.shadowElement.attr({
                            path: m.silhuette
                        }), d.isFunnel ? r.funnel2D.attr({
                            path: m.silhuette
                        }) : (r.lighterHalf.attr({
                            path: m.lighterHalf
                        }), r.darkerHalf.attr({
                            path: m.darkerHalf
                        }), r.borderElement.attr({
                            path: m.silhuette
                        }))), v &&
                        (d.isFunnel ? r.funnel2D.attr("fill", q(u(d.color, a(d.opacity, 100)))) : (!1 === d.use3DLighting ? m = t = d.color : (m = W(d.color, 80), t = N(d.color, 80)), r.lighterHalf.attr("fill", q(u(t, a(d.opacity, 100)))), r.darkerHalf.attr("fill", q(u(m, a(d.opacity, 100))))))) : (z && (m = p(d.x, d.y, d.R1, d.R2, d.h, d.r3dFactor, d.is2D), r.shadowElement.attr("path", m.silhuette), d.isFunnel ? (r.front.attr("path", m.front), r.back.attr("path", m.back), r.toptop && m.top && r.toptop.attr("path", m.top)) : (r.front.attr("path", m.front), r.toptop.attr("path", m.top),
                        r.topLight.attr("path", m.topLight), r.topLight1.attr("path", m.topLight1), r.centerLight.attr("path", m.centerLight), r.centerLight1.attr("path", m.centerLight1))), v && (m = d.color, t = d.opacity, d.isFunnel ? (v = N(m, 60), z = W(m, 60), r.back.attr("fill", q({
                            FCcolor: {
                                color: z + J + v + J + m,
                                alpha: t + J + t + J + t,
                                ratio: "0,60,40",
                                angle: 0
                            }
                        })), r.front.attr("fill", q({
                            FCcolor: {
                                color: m + J + v + J + z,
                                alpha: t + J + t + J + t,
                                ratio: "0,40,60",
                                angle: 0
                            }
                        })), r.toptop && r.toptop.attr("fill", q({
                            FCcolor: {
                                color: v + J + z,
                                alpha: t + J + t,
                                ratio: "0,100",
                                angle: - 65
                            }
                        }))) : (v = N(m,
                        80), g = N(m, 70), z = W(m, 80), l = "0," + t, y = m + J + g, d = 5 / (d.R1 * d.r3dFactor) * 100, r.centerLight.attr("fill", q({
                            FCcolor: {
                                color: y,
                                alpha: l,
                                ratio: "0,100",
                                angle: 0
                            }
                        })), r.centerLight1.attr("fill", q({
                            FCcolor: {
                                color: y,
                                alpha: l,
                                ratio: "0,100",
                                angle: 180
                            }
                        })), r.topLight.attr("fill", q({
                            FCcolor: {
                                color: g + J + g + J + m + J + m,
                                alpha: t + J + t + J + 0 + J + 0,
                                ratio: "0,50," + d + J + (50 - d),
                                angle: - 45
                            }
                        })), r.topLight1.attr("fill", q({
                            FCcolor: {
                                color: g + J + m + J + z,
                                alpha: t + J + t + J + t,
                                ratio: "0,50,50",
                                angle: 0
                            }
                        })), r.front.attr("fill", q({
                            FCcolor: {
                                color: m + J + m + J + z + J + z,
                                alpha: t +
                                J + t + J + t + J + t,
                                ratio: "0,50,0,50",
                                angle: 0
                            }
                        })), r.toptop.attr("fill", q({
                            FCcolor: {
                                color: v + J + m + J + z + J + z,
                                alpha: t + J + t + J + t + J + t,
                                ratio: "0,25,30,45",
                                angle: - 45
                            }
                        })))))
                    }
                    return r
                }, l = function() {
                    var c = this.shadowElement;
                    l && c.shadow.apply(c, arguments)
                }, r = function(c, b, a) {
                    var h = c.chart.get(ia, v), q = h.duration, p = h.dummyObj, f = h.animObj, g = h.animType;
                    return function(h, l, r) {
                        if (h = a[h])
                            return h.animateWith(p, f, {
                                path: l
                            }, q, g, c.postPlotCallback);
                        r = r || y;
                        c.postPlotCallback();
                        return b.path(l, a).attr(r)
                    }
                };
                return function(c, b, h, q, f, d, x,
                z, v, A, w, y) {
                    var e = this.chart.graphics.datasetGroup, G;
                    "object" === typeof c && (b = c.y, h = c.R1, q = c.R2, f = c.h, d = c.r3dFactor, x = c.gStr, z = c.is2D, y = c.use3DLighting, v = c.renderer, w = c.isHollow, A = c.isFunnel, G = c.graphics, c = c.x);
                    d = a(d, .15);
                    c = {
                        x: c,
                        y: b,
                        R1: h,
                        R2: q,
                        h: f,
                        r3dFactor: d,
                        is2D: z,
                        use3DLighting: y,
                        isHollow: w,
                        isFunnel: A,
                        renderer: v
                    };
                    b = p(c);
                    h = "silhuette lighterHalf darkerHalf centerLight centerLight1 front toptop topLight topLight1 shadowElement funnel2D back".split(" ");
                    if (G) {
                        if (q = G._3dAttr, q.isFunnel !== c.isFunnel || q.is2D !==
                        c.is2D || q.isHollow !== c.isHollow)
                            for (f = 0, w = h.length; f < w; f++)
                                if (d = h[f], q = G[d])
                                    delete G[d], q.remove()
                    } else 
                        G = void 0;
                    x = G || v.group(x, e);
                    x.toFront();
                    x.Shapeargs = b;
                    v = r(this, v, x, "easeIn");
                    x.shadowElement = v("shadowElement", b.silhuette, {
                        fill: m,
                        stroke: "none"
                    });
                    x._attr = x._attr || x.attr;
                    x.attr = g;
                    x.shadow = l;
                    x._3dAttr = c;
                    A ? z ? x.funnel2D = v("funnel2D", b.silhuette) : (x.back = v("back", b.back, {
                        "stroke-width": 0,
                        stroke: "none"
                    }), x.front = v("front", b.front, {
                        "stroke-width": 0,
                        stroke: "none"
                    }), b.top && (x.toptop = v("toptop", b.top, {
                        "stroke-width": 0,
                        stroke: "none"
                    }))) : z ? (x.lighterHalf = v("lighterHalf", b.lighterHalf, {
                        "stroke-width": 0
                    }), x.darkerHalf = v("darkerHalf", b.darkerHalf, {
                        "stroke-width": 0
                    }), x.borderElement = v("borderElement", b.silhuette, {
                        fill: m,
                        stroke: "none"
                    })) : (x.front = v("front", b.front, {
                        "stroke-width": 0
                    }), x.centerLight = v("centerLight", b.centerLight, {
                        "stroke-width": 0
                    }), x.centerLight1 = v("centerLight1", b.centerLight1, {
                        "stroke-width": 0
                    }), x.toptop = v("toptop", b.top, {
                        "stroke-width": 0
                    }), x.topLight = v("topLight", b.topLight, {
                        "stroke-width": 0
                    }), x.topLight1 =
                    v("topLight1", b.topLight1, {
                        "stroke-width": 0
                    }));
                    return x
                }
            }(), utils : function(c) {
                function b() {
                    var c = [], a = 0;
                    this.set = function(b, h) {
                        a++;
                        c[b] = h
                    };
                    this.get = function(b) {
                        return c[b]
                    };
                    this.getAll = function() {
                        return c
                    };
                    this.mergeWith = function(a) {
                        var h, q, p = c.slice(0);
                        if (!(a instanceof Array))
                            if (a instanceof b)
                                a = a.getAll();
                            else 
                                return;
                        for (h in a)
                            q = a[h], p[h] || (p[h] = q);
                        return p
                    };
                    this.getEffectiveLength = function() {
                        return a
                    }
                }
                function a(c) {
                    this.distributionLength = c;
                    this.distributedMatrix = [];
                    this.altDistributedMatrix =
                    [];
                    this.nonDistributedMatrix = {};
                    this.forcePushObj = {};
                    this.flags = {
                        exhaustion: !1
                    }
                }
                b.prototype.constructor = b;
                a.prototype.constructor = a;
                a.prototype.push = function(c, b) {
                    this.nonDistributedMatrix[b] = this.nonDistributedMatrix[b] || [];
                    this.nonDistributedMatrix[b].push(c)
                };
                a.prototype.forcePush = function(c, b) {
                    this.forcePushObj[b] = c
                };
                a.prototype.distribute = function(a) {
                    var h = c, q=!0, p = new b, f = new b, l = new b, g = new b, r = this.flags, m, d, t, x, v, z = 0, A = h.components.data;
                    if (a) {
                        if (0 < A.length - this.distributionLength)
                            for (d in this.nonDistributedMatrix)
                                for (h =
                                this.nonDistributedMatrix[d], x = 1; x < h.length; x++)
                                    v = h[x], v.dontPlot=!0, v.displayValue = n
                    } else if (0 < A.length - 2 * this.distributionLength)
                        for (d in this.nonDistributedMatrix)
                            for (h = this.nonDistributedMatrix[d], x = 1; x < h.length - 1; x++)
                                v = h[x], v.dontPlot=!0, v.displayValue = n;
                    if (A.length > this.distributionLength&&!a) {
                        r.exhaustion=!0;
                        for (d in this.nonDistributedMatrix)
                            for (h = this.nonDistributedMatrix[d], x = 0, a = h.length; x < a; x++)
                                v = h[x], v.dontPlot ? q ? m = l : m = g : (q ? m = p : m = f, m.getEffectiveLength() > parseInt(d, 10) ? v.distributionFactor =
                                m.getEffectiveLength() - 1 - d : v.distributionFactor = 0), m.set(z++, v), q=!q;
                        this.distributedMatrix = p.mergeWith(l);
                        this.altDistributedMatrix = f.mergeWith(g)
                    } else {
                        for (t in this.nonDistributedMatrix)
                            for (h = this.nonDistributedMatrix[t], x = 0, a = h.length; x < a; x++)
                                v = h[x], v.dontPlot ? m = l : (m = p, m.getEffectiveLength() > parseInt(t, 10) ? v.distributionFactor = m.getEffectiveLength() - 1 - t : v.distributionFactor = 0), m.set(z++, v);
                        this.distributedMatrix = p.mergeWith(l)
                    }
                };
                a.prototype.getDistributedResult = function() {
                    var b = c, a = b.chart, h = b.conf.alignmentType,
                    b = [], q = a.isLegendRight;
                    a.components.legend.config.width || (q = 0);
                    a.isLegendRight = q;
                    this.distribute(q);
                    q ? (a = h["default"], b.push(this.distributedMatrix)) : (a = this.flags.exhaustion ? h.alternate : h["default"], this.flags.exhaustion ? [].push.call(b, this.distributedMatrix, this.altDistributedMatrix) : b.push(this.distributedMatrix));
                    return {
                        forceMatrix: this.forcePushObj,
                        suggestion: a,
                        matrix: b
                    }
                };
                return {
                    DistributionMatrix: a,
                    setContext: function(b) {
                        c = b
                    },
                    invokeHookFns: function() {
                        var b, a = [], h = c;
                        switch (arguments.length) {
                        case 3:
                            h =
                            arguments[2];
                        case 2:
                            a = arguments[1];
                        case 1:
                            b = arguments[0];
                            break;
                        default:
                            return 
                        }
                        b && "function" === typeof b && b.apply(h, a)
                    },
                    copyProperties: function(b, a, h) {
                        function q(b) {
                            return "string" === typeof b && 0 === b.indexOf("$") ? (b = b.substring(1), a[b]) : "function" === typeof b ? b.call(c, a) : b
                        }
                        var p, f, l, g, m, r, x, d = function() {};
                        p = 0;
                        for (f = h.length; p < f; p++)
                            l = h[p], g = l[0], m = l[1], r = l[2], x = q(l[3]), l = l[4] || d, a[m] = r(b[g], x), l(a)
                    },
                    sortObjArrByKey: function(c, b) {
                        return c.sort(function(c, a) {
                            return Math.abs(a[b]) - Math.abs(c[b])
                        })
                    },
                    getSumValueOfObjArrByKey: function(c,
                    b) {
                        var a, h, q = 0;
                        a = 0;
                        for (h = c.length; a < h; a++)
                            q += parseFloat(c[a][b], 10);
                        return q
                    }
                }
            }, slice: function(c, a, h, q) {
                a = this.plotItem;
                var p = this.datasetStore, l = p.chart, f = l.get(ia, v);
                h = f.duration;
                var g = f.dummyObj, m = f.animObj, f = f.animType, r = p.conf, x = r.slicingDistance / 2, d = 0, A = 0, w = p.graphics.plotItems, y = w.length, e, G, H, W, F;
                if (!r.sliceLock)
                    for (r.sliceLock = 1, a && z.call(a.trackerObj, l, c, "dataplotclick"), q = a.sliced = void 0 !== q && null !== q ? q : !a.sliced, W =- x, F = function() {
                        return function() {
                            r.sliceLock = 0;
                            b.raiseEvent("SlicingEnd",
                            {
                                slicedState: q,
                                data: p.getPlotData(H)
                            }, l.chartInstance)
                        }
                    }, d = 0; d < y; d += 1)
                        c = w[d], c !== a ? G = c.sliced=!1 : (G=!0, H = d), c.graphic && (e = c.dy, e =- e, q && (c.x < a.x ? (e += W, A += 1) : c.x == a.x ? A ? d == y - 1 && (e += .5 * x) : e += .5*-x : e += x), c.graphic.attr({
                            transform: "t0," + c.dy
                        }), c.dy += e, e = {
                            transform: "...t0," + e
                        }, G && b.raiseEvent("SlicingStart", {
                            slicedState: !q,
                            data: p.getPlotData(H)
                        }, l.chartInstance), c.graphic.animateWith(g, m, e, h, f, G && F(q, H)), c.dataLabel && c.dataLabel.animateWith(g, m, e, h, f), c.connector && c.connector.animateWith(g, m, e, h, f), c.trackerObj &&
                        c.trackerObj.animateWith(g, m, e, h, f), 1 == d&&!w[0].graphic && w[0].dataLabel && w[0].dataLabel.animateWith(g, m, e, h, f))
            }, legendClick: function(c) {
                this.slice.call(c)
            }, getEventArgs: function(c) {
                c = this.components.data[c.configuration.index] || {};
                return {
                    alpha: c.alpha,
                    value: c.y,
                    color: c.color,
                    borderColor: c.borderColor,
                    borderWidth: c.borderWidth,
                    link: c.link,
                    displayValue: c.displayValue,
                    datasetIndex: c.datasetIndex,
                    toolText: c.toolText,
                    label: c.categoryLabel
                }
            }
        }
        ])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-funnel",
    function() {
        var b = this.hcLib, k = b.pluckNumber, d = b.BLANKSTRING, a = b.setLineHeight, n = b.plotEventHandler, L = b.schedular, D = b.preDefStr, u = D.configStr, e = D.animationObjStr, C = D.POSITION_START, N = D.POSITION_END, D = D.POSITION_MIDDLE, J = {
            right: N,
            left: C,
            middle: D,
            start: C,
            end: N,
            center: D,
            undefined: d,
            BLANK: d
        };
        E.register("component", ["dataset", "Funnel", {
            type: "funnel",
            LABEL_PLACEMENT_ITERATOR_INDEX_START: 1,
            configure: function() {
                var b = this.chart, a = b.components.numberFormatter, d = this.utils(this).sortObjArrByKey, e = b.jsonData ?
                b.jsonData.chart: {}, b = this.JSONData.data, u, c, F;
                if (this._checkValidData(b)) {
                    u = 0;
                    for (F = b.length; u < F; u++)(c = b[u]) 
                        && void 0 !== c.value && (c.value = a.getCleanValue(c.value, !0));
                    a =+ (void 0 === e.streamlineddata ? 1 : e.streamlineddata);
                    this.JSONData.data = a ? d(b, "value") : b;
                    a && (b[0].pseudoPoint=!0);
                    this.addLegend()
                }
            },
            configureSpecifics: function() {
                var b = this.chart, a = this.conf, d = b.jsonData ? b.jsonData.chart: {}, e = this.utils(this).copyProperties;
                e(d, a, [["streamlineddata", "streamLinedData", k, 1], ["funnelyscale", "yScale", k,
                void 0, function(b) {
                    var c = b.yScale;
                    b.yScale = 0 <= c && 40 >= c ? c / 200: .2
                }
                ], ["usesameslantangle", "useSameSlantAngle", k, function(b) {
                    return b.streamLinedData ? 0 : 1
                }
                ], ["ishollow", "isHollow", k, void 0, function(b) {
                    void 0 === b.isHollow && (b.isHollow = b.streamLinedData ? 1: 0)
                }
                ]]);
                b.config.PLOT_COLOR_INDEX_START = a.streamLinedData?-1 : 0
            },
            preDrawingHook: function() {
                var b = this.components.data, a = this.conf;
                a.streamLinedData || b.splice(0, 0, {
                    displayValue: d,
                    y: a.sumValue
                })
            },
            prePointProcessingHookFn: function(b) {
                var d = this.chart, e = d.config,
                u = this.conf, n = e.canvasWidth, c = d.linkedItems.smartLabel, F=!u.streamLinedData, g, H;
                (g = b[0]) && (g.pseudoPoint=!0);
                g && g.displayValue && (c.useEllipsesOnOverflow(d.config.useEllipsesWhenOverflow), c.setStyle(g.style), a(g.style), d = parseFloat(g.style.lineHeight.match(/^\d+/)[0] || u.lineHeight, 10), H = c.getOriSize(g.displayValue).height, n = c.getSmartText(g.displayValue, n, H), g.displayValue = n.text, n.tooltext && (g.originalText = n.tooltext), g.labelWidth = c.getOriSize(n.text).width, e.marginTop += d + 4);
                u.totalValue = F ? b[0].y -
                b[1].y : 0;
                u.offsetVal = function(c) {
                    return F?-(b[c + 1] && b[c + 1].y || 0) : g.y
                }
            },
            getPointInContext: function() {
                function b(a) {
                    this.displayValue = a.displayValue;
                    this.displayValueArgs = a.displayValueArgs;
                    this.style = a.style;
                    this.appliedStyle = a.appliedStyle;
                    this.categoryLabel = a.categoryLabel;
                    this.toolText = a.toolText;
                    this.legendCosmetics = a.legendCosmetics;
                    this.showInLegend = a.showInLegend;
                    this.y = a.y;
                    this.shadow = a.shadow;
                    this.smartTextObj = a.smartTextObj;
                    this.color = a.color;
                    this.legendItemId = a.legendItemId;
                    this.name = a.name;
                    this.alpha = a.alpha;
                    this.rawColor = a.rawColor;
                    this.rawAlpha = a.rawAlpha;
                    this.legendColor = a.legendColor;
                    this.borderColor = a.borderColor;
                    this.borderWidth = a.borderWidth;
                    this.link = a.link;
                    this.isSliced = a.isSliced;
                    this.doNotSlice = a.doNotSlice;
                    this.hoverEffects = a.hoverEffects;
                    this.rolloverProperties = a.rolloverProperties
                }
                var a = this;
                b.upperRadiusFactor = 1;
                b.prototype.getModifiedCurrentValue = function() {};
                b.prototype.getRatioK = function(b, d, e, c) {
                    b = a.conf;
                    var u = b.useSameSlantAngle;
                    return b.streamLinedData ? this.y ? u ?
                    this.y / c : Math.sqrt(this.y / c) : 1 : .2 + d * e
                };
                b.prototype.getLowestRadiusFactor = function(b) {
                    var d = a.conf, e = d.useSameSlantAngle;
                    return d.streamLinedData ? this.y ? e ? this.y / b : Math.sqrt(this.y / b) : 1 : .2
                };
                return b
            },
            datasetCalculations: function(b) {
                var a = this.conf, d = this.chart.components.numberFormatter, e, u, c, F = {}, g = a.streamLinedData, H = a.percentOfPrevious;
                F.highestValue = Number.NEGATIVE_INFINITY;
                F.refreshedData = [];
                a = F.sumValue = F.countPoint = 0;
                for (e = b.length; a < e; a++)
                    u = b[a], u.vline || (u.cleanValue = c = Math.abs(d.getCleanValue(u.value,
                    !0)), null !== c && (F.hasValidPoint=!0, F.highestValue = F.highestValue || c, F.refreshedData.push(u), F.sumValue += c, F.countPoint += 1, F.highestValue = Math.max(F.highestValue, c)));
                g && (F.sumValue = F.highestValue, H && (F.prevPerValReq=!0));
                return F
            },
            draw: function() {
                this._configure();
                var a = this, d = a.chart, e = d.graphics, u = d.getJobList(), k = d.config, c = a.conf, F = a.utils(a), g = a.trackerArgs = [], H = F.getSumValueOfObjArrByKey, B = F.DistributionMatrix, f = a.calculatePositionCoordinate, r = k.marginTop, y = k.marginBottom, F = a.components.data,
                W, p, ia, v = e.datalabelsGroup, l = c.streamLinedData, ra, D = 2;
                W = F.length;
                var m = c.maxValue = F[0].y;
                ia = c.minValue = F[W - 1].y;
                var h = ra = 0, w = c.lineHeight, q = Math.floor, e = Math.min, z, A = a.graphics.plotItems, x = [], na;
                a.labelDrawingConfig = a.labelDrawingConfig || [];
                a.labelDrawingConfig.length = 0;
                if (c.sumValue && (a.preDrawingSpaceManagement(), a.hide(a.graphics.plotItems), a.rolloverResponseSetter = function(c, a) {
                    return function(b) {
                        c.graphic.attr(a);
                        n.call(this, d, b, "DataPlotRollOver")
                    }
                }, a.rolloutResponseSetter = function(c, a) {
                    return function(b) {
                        c.graphic.attr(a);
                        n.call(this, d, b, "DataPlotRollOut")
                    }
                }, a.legendClickHandler = function(c) {
                    return function() {
                        a.legendClick(c, !0, !1)
                    }
                }, a.animateFunction = function(c) {
                    return function() {
                        d._animCallBack();
                        c.attr({
                            opacity: 1
                        })
                    }
                }, a.postPlotCallback = function() {}, k.canvasTop += k.marginTop - r, k.effCanvasHeight = ra = k.canvasHeight - (k.marginTop + k.marginBottom) + (r + y), k.effCanvasWidth = r = k.width - (k.marginLeft + k.marginRight), p = c.drawingRadius = r / D, c.x = p + k.canvasLeft, D = c.slicingDistance, y = D / 2, !(l && 2 > W))) {
                    l ? (ia = ra / (m - ia), m = (H = H(F, "value")) ?
                    ra / H : ra) : m = ia = m ? ra / m : ra;
                    c.unitHeight = ia;
                    c.lastRadius = p;
                    c.globalMinXShift = 0;
                    H = c.alignmentType = {};
                    H["default"] = 1;
                    H.alternate = 2;
                    ia = new B(q(ra / w));
                    for (B = 0; B < W; B++)
                        p = F[B], !l && 0 === B ||!l && B === W - 1 ? ia.forcePush(p, B) : (ra = p.y * m, h += p.y * m, ra = h - ra + ra / 2, ra = q(ra / w), ia.push(p, ra));
                    l = ia.getDistributedResult();
                    F.length = 0;
                    if (void 0 === l.matrix[1])
                        [].push.apply(F, l.matrix[0]);
                    else 
                        for (h = l.matrix[0], w = l.matrix[1], W = Math.max(h.length, w.length), B = 0; B < W; B++)
                            ra = h[B], q = w[B], F.push(ra ? ra : q);
                    h = Object.keys(l.forceMatrix);
                    if (0 <
                    h.length)
                        for (z in l.forceMatrix)[].splice.apply(F, [parseInt(z, 10), 0].concat(l.forceMatrix[z])
                            );
                    switch (l.suggestion) {
                    case H["default"]:
                        f.call(a, F, !1);
                        break;
                    case H.alternate:
                        c.labelAlignment = H.alternate, p = r / 3, k.canvasLeft = k.canvasWidth / 2 - p, c.x = k.canvasLeft + p, f.call(a, F, !0)
                    }
                    if (k = c.noOfGap)
                        c.perGapDistance = e(1.5 * y, D / k), c.distanceAvailed = y;
                    v.trackTooltip(!0);
                    B = F.length;
                    A || (a.graphics.plotItems = []);
                    c.alreadyPlotted && (a.postPlotCallback = function() {
                        na || (na=!0, a.animateFunction(v)())
                    });
                    for (; B--;)
                        x.push(a.drawIndividualPlot(F[B],
                        B));
                    !c.alreadyPlotted && a.animateElements(x, "graphic", [], {
                        pre: {
                            opacity: 0
                        },
                        post: {
                            opacity: 100
                        }
                    }, a.animateFunction(v));
                    c.connectorEndSwitchHistoryY = {};
                    for (B = F.length; B--;)
                        g.push(F[B]);
                    u.labelDrawID.push(L.addJob(a.drawAllLabels.bind(a), b.priorityList.label));
                    u.trackerDrawID.push(L.addJob(a.drawAllTrackers.bind(a), b.priorityList.tracker));
                    c.alreadyPlotted=!0
                }
            },
            drawIndividualPlot: function(a, b) {
                var G = this.conf, k = this.graphics.plotItems, n = a.y, c = a.displayValue, F = G.isSliced, g = a.labelAline, H = a.appliedStyle,
                B = a.style, f = this.components.data, r = this.chart, y = r.graphics.trackerGroup, W = r.components.paper, p=!!a.link, ia = G.distanceAvailed, v = r.components.legend, r = r.get(u, e).duration, l = this.labelDrawingConfig, F = F ? 1 : a.isSliced, B = {
                    text: c,
                    ishot: !0,
                    direction: d,
                    cursor: p ? "pointer": d,
                    x: 0,
                    y: 0,
                    fill: H && H.color || B && B.color || this._chartLevelAttr.color,
                    "text-anchor": J[g]
                };
                if (null !== n && void 0 !== n && a.shapeArgs)
                    return (g = k[b]) ? (a.plot = g, a.shapeArgs.graphics = g.graphic, a.shapeArgs.animationDuration = r, g.graphic = this.pyramidFunnelShape(a.shapeArgs).attr({
                        fill: a.color,
                        "stroke-width": a.borderWidth,
                        stroke: a.borderColor
                    }), g.graphic.show()) : (a.shapeArgs.graphics = g, a.shapeArgs.animationDuration = r, a.plot = g = k[b] = {
                        graphic: this.pyramidFunnelShape(a.shapeArgs).attr({
                            fill: a.color,
                            "stroke-width": a.borderWidth,
                            stroke: a.borderColor
                        }),
                        trackerObj: W.path(y)
                    }), l[b] = k = {
                        args: B,
                        css: H,
                        point: a
                    }, G.showTooltip ? g.trackerObj.tooltip(a.toolText) : g.trackerObj.tooltip(!1), g.value = n, g.displayValue = c, g.sliced=!!F, g.cursor = p ? "pointer" : d, g.x = a.x, g.index = b, v.configureItems(f[b].legendItemId, {
                        legendClickFN: this.legendClickHandler({
                            datasetStore: this,
                            plotItem: g
                        })
                    }), n = c = {}, a.hoverEffects && (n = {
                        color: a.rawColor,
                        opacity: a.rawAlpha,
                        "stroke-width": a.borderWidth,
                        stroke: a.borderColor
                    }, c = a.rolloverProperties, c = {
                        color: c.color,
                        opacity: c.alpha,
                        "stroke-width": c.borderWidth,
                        stroke: c.borderColor
                    }), f = {
                        datasetStore: this,
                        plotItem: g
                    }, g.trackerObj.unclick(this.slice), !a.doNotSlice && g.trackerObj.click(this.slice, f), g.trackerObj.mouseup(this.plotMouseUp, g), g.trackerObj.hover(this.rolloverResponseSetter(g, c), this.rolloutResponseSetter(g, n)), k.context = f, k.actions = {
                        click: this.slice,
                        hover: [this.rolloverResponseSetter(g, c), this.rolloutResponseSetter(g, n)]
                    }, g.dy = 0, G.noOfGap && (ia && (g._startTranslateY = n = "t0," + ia, g.dy = g.distanceAvailed = ia, g.graphic.attr({
                        transform: n
                    }), k.transform = n), G.slicingGapPosition[a.x] && (G.distanceAvailed -= G.perGapDistance)), g && (g.point = a), g;
                l[b] = {
                    args: B,
                    css: H,
                    point: a
                }
            },
            getTooltipMacroStub: function(a) {
                var b = this.conf, d = this.chart.components.numberFormatter, e = this.__base__, u;
                b.streamLinedData && (u = b.percentOfPrevious ? a.pValue : d.percentValue(a.dataValue / a.prevValue *
                100));
                e = e.getTooltipMacroStub(a);
                e.percentValue = b.percentOfPrevious ? d.percentValue(a.dataValue / a.highestValue * 100) : a.pValue;
                e.percentOfPrevValue = u;
                return e
            }
        }, "FunnelPyramidBase"])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-pyramid", function() {
        var b = this, k = b.hcLib, d = k.BLANKSTRING, a = k.pluckNumber, n = k.preDefStr, L = n.configStr, D = n.animationObjStr, u = n.POSITION_START, e = n.POSITION_END, C = n.POSITION_MIDDLE, n = b.window, N = /msie/i.test(n.navigator.userAgent)&&!n.opera ? .002: 1E-6, J = k.plotEventHandler,
        K = k.schedular, I = {
            right: e,
            left: u,
            middle: C,
            start: u,
            end: e,
            center: C,
            undefined: d,
            BLANK: d
        };
        E.register("component", ["dataset", "Pyramid", {
            type: "pyramid",
            LABEL_PLACEMENT_ITERATOR_INDEX_START: 0,
            configure: function() {
                this._checkValidData(this.JSONData.data) && this.addLegend()
            },
            configureSpecifics: function() {
                var b = this.chart, d = this.conf, e = b.jsonData ? b.jsonData.chart: {}, c = this.utils(this).copyProperties;
                c(e, d, [["pyramidyscale", "yScale", a, void 0, function(c) {
                    var a = c.yScale;
                    c.yScale = 0 <= a && 40 >= a ? a / 200: .2
                }
                ], ["use3dlighting",
                "use3DLighting", a, 1]]);
                b.config.PLOT_COLOR_INDEX_START = 0
            },
            preDrawingHook: function() {},
            draw: function() {
                this._configure();
                var a = this, b = a.chart, d = b.getJobList(), c = b.config, e = a.conf, g = a.utils(a).DistributionMatrix, H = a.trackerArgs = [], u = a.calculatePositionCoordinate, f = c.marginTop, r = c.marginBottom, y = a.components.data, W, p, ia, v = b.graphics.datalabelsGroup, l, n = 2, D = y.length, m, h = m = 0, w = e.lineHeight, q = Math.floor, z = Math.min, A, x = a.graphics.plotItems, na = [], ta;
                if (e.sumValue) {
                    a.labelDrawingConfig = a.labelDrawingConfig ||
                    [];
                    a.labelDrawingConfig.length = 0;
                    a.preDrawingSpaceManagement();
                    a.hide(a.graphics.plotItems);
                    a.rolloverResponseSetter = function(a, c) {
                        return function(h) {
                            a.graphic.attr(c);
                            J.call(this, b, h, "DataPlotRollOver")
                        }
                    };
                    a.rolloutResponseSetter = function(a, c) {
                        return function(h) {
                            a.graphic.attr(c);
                            J.call(this, b, h, "DataPlotRollOut")
                        }
                    };
                    a.legendClickHandler = function(c) {
                        return function() {
                            a.legendClick(c, !0, !1)
                        }
                    };
                    a.animateFunction = function(a) {
                        return function() {
                            b._animCallBack();
                            a.attr({
                                opacity: 1
                            })
                        }
                    };
                    a.postPlotCallback =
                    function() {};
                    c.canvasTop += c.marginTop - f;
                    c.effCanvasHeight = W = c.canvasHeight - (c.marginTop + c.marginBottom) + (f + r);
                    c.effCanvasWidth = f = c.width - (c.marginLeft + c.marginRight);
                    l = e.drawingRadius = f / n;
                    e.x = l + c.canvasLeft;
                    n = e.slicingDistance;
                    r = n / 2;
                    l = Math.atan(f / 2 / W);
                    e.unitHeight = ia = W / e.sumValue;
                    e.lastRadius = 0;
                    e.globalMinXShift = Math.floor(w / Math.cos(l));
                    l = e.alignmentType = {};
                    l["default"] = 1;
                    l.alternate = 2;
                    A = new g(q(W / w));
                    g = 0;
                    for (W = D; g < W; g++)
                        p = y[g], m = p.y * ia, h += p.y * ia, m = h - m + m / 2, m = q(m / w), A.push(p, m);
                    h = A.getDistributedResult();
                    y.length = 0;
                    if (void 0 === h.matrix[1])
                        [].push.apply(y, h.matrix[0]);
                    else 
                        for (w = h.matrix[0], q = h.matrix[1], W = Math.max(w.length, q.length), g = 0; g < W; g++)
                            m = w[g], ia = q[g], y.push(m ? m : ia);
                    switch (h.suggestion) {
                    case l["default"]:
                        u.call(a, y, !1);
                        break;
                    case l.alternate:
                        e.labelAlignment = l.alternate, e.drawingRadius = l = f / 3, c.canvasLeft = c.canvasWidth / 2 - l, e.x = c.canvasLeft + l, u.call(a, y, !0)
                    }
                    if (c = e.noOfGap)
                        e.perGapDistance = z(1.5 * r, n / c), e.distanceAvailed = r;
                    v.trackTooltip(!0);
                    g = y.length;
                    x || (x = a.graphics.plotItems = []);
                    e.alreadyPlotted &&
                    (a.postPlotCallback = function() {
                        ta || (ta=!0, a.animateFunction(v)())
                    });
                    for (; g--;)
                        na.push(a.drawIndividualPlot(y[g], g));
                    !e.alreadyPlotted && a.animateElements(na, "graphic", [], {
                        pre: {
                            opacity: N
                        },
                        post: {
                            opacity: 100
                        }
                    }, a.animateFunction(v));
                    x.splice(D, x.length - D);
                    e.connectorEndSwitchHistoryY = {};
                    for (g = y.length; g--;)
                        H.push(y[g]);
                    d.labelDrawID.push(K.addJob(a.drawAllLabels.bind(a), k.priorityList.label));
                    d.trackerDrawID.push(K.addJob(a.drawAllTrackers.bind(a), k.priorityList.tracker));
                    e.alreadyPlotted=!0
                }
            },
            calculatePositionCoordinate: function(a,
            d) {
                var e = this.conf, c = e.is2d, F = e.x, g = this.graphics.plotItems || [], H = this.chart, B = H.config, f = B.canvasTop, r = e.unitHeight, y = e.labelDistance, W = e.showLabelsAtCenter, p = .3 * B.style.fontSize, ia = e.yScale, v = e.blankSpace, l = e.lastRadius, k = H.linkedItems.smartLabel, n, m, h, w = a.length - 1, q=!1, z = 0, A, x, na = e.lineHeight, ta = 0;
                n = {
                    flag: !1,
                    point: void 0,
                    sLabel: void 0,
                    setAll: function(a,
                    c,
                    b) {
                        this.flag = a;
                        this.point = c;
                        this.sLabel = b
                    }
                };
                var t = {
                    point: void 0,
                    sLabel: void 0,
                    set: function(a,
                    c) {
                        return function(b,
                        h) {
                            var q,
                            p;
                            b.dontPlot || (this.point &&
                            this.sLabel ? (q = a(this.point,
                            this.sLabel),
                            p = a(b,
                            h),
                            c(q,
                            p) && (this.point = b,
                            this.sLabel = h)): (this.point = b,
                            this.sLabel = h))
                        }
                    }
                }, pa = {}, D = {}, R = {}, I = {}, Q = H.config.width - 2, K = e.slicingGapPosition = {};
                b.extend(pa, n);
                b.extend(D, n);
                pa.setAll = function(a, c, b) {
                    var h = this.point, q = this.sLabel;
                    this.flag = a;
                    h && q ? (a = h.labelX - (q.oriTextWidth - q.width), h = c.labelX - (b.oriTextWidth - b.width), a > h && (this.point = c, this.sLabel = b)): (this.point = c, this.sLabel = b)
                };
                D.setAll = function(a, c, b) {
                    var h = this.point, q = this.sLabel;
                    this.flag = a;
                    h && q ?
                    (a = h.labelX + q.oriTextWidth, h = c.labelX + b.oriTextWidth, a < h && (this.point = c, this.sLabel = b)): (this.point = c, this.sLabel = b)
                };
                b.extend(R, t);
                b.extend(I, t);
                R.set = function() {
                    return t.set.apply(R, [function(a) {
                        return a.labelX
                    }, function(a, c) {
                        return a > c?!0: !1
                    }
                    ])
                }();
                I.set = function() {
                    return t.set.apply(I, [function(a, c) {
                        return a.labelX + c.oriTextWidth
                    }, function(a, c) {
                        return a < c?!0: !1
                    }
                    ])
                }();
                e.noOfGap = 0;
                k.useEllipsesOnOverflow(B.useEllipsesWhenOverflow);
                B = 0;
                for (n = a.length; B < n; B++)
                    if (m = a[B])
                        m.x = B, g[B] && (m.isSliced = g[B].sliced ||
                        !!m.isSliced||!!e.isSliced), d && (q=!q), m.isSliced && ((h = m.x)&&!K[h] && (K[h]=!0, e.noOfGap += 1), h < w && (K[h + 1]=!0, e.noOfGap += 1)), k.setStyle(m.style), m.oriText = m.displayValue, A = A = k.getSmartText(m.displayValue, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY), ta += m.y, x = e.drawingRadius * ta / e.sumValue, h = r * m.y, m.shapeArgs = {
                            x: F,
                            y: f,
                            R1: l,
                            R2: x,
                            h: h,
                            r3dFactor: ia,
                            gStr: "point",
                            is2D: c,
                            use3DLighting: !!e.use3DLighting,
                            renderer: H.components.paper
                        }, W ? (m.labelAline = C, m.labelX = F, m.labelY = (c ? f : f + ia * l) + h / 2 + p) : (m.labelAline = u,
                        m.alignmentSwitch = q, m.distributionFactor = m.distributionFactor || 0, q ? (m.labelX = F - (y + (x + l) / 2 + v + A.width), m.labelX -= m.distributionFactor * e.globalMinXShift, R.set(m, A)) : (m.labelX = F + y + (x + l) / 2 + v, m.labelX += m.distributionFactor * e.globalMinXShift, I.set(m, A)), z = m.distributionFactor * na, m.labelY = f + p + h / 2 + z), d && (q && 0 > m.labelX ? (l = m.labelX + A.width, l = k.getSmartText(m.displayValue, l, Number.POSITIVE_INFINITY, !0), m.labelX = 2, m.isLabelTruncated=!0, m.displayValue = l.text, m.virtualWidth = l.maxWidth, pa.setAll(!0, m, l)) : !q && m.labelX +
                        A.width > Q && (l = k.getSmartText(m.displayValue, Q - m.labelX, Number.POSITIVE_INFINITY, !0), m.isLabelTruncated=!0, m.displayValue = l.text, m.virtualWidth = l.maxWidth, D.setAll(!0, m, l)), m.pWidth = m.virtualWidth || A.width, z = m.distributionFactor * na, m.labelY = f + p + h / 2 + z), f += h, m.plotX = F, m.plotY = f - h / 2, l = x, m.virtualWidth = m.virtualWidth || A.width;
                this.findBestPosition.call(this, a, {
                    lTrimmedInfo: pa,
                    rTrimmedInfo: D,
                    lLargestLabel: R,
                    rLargestLabel: I
                })
            }, getPointInContext: function() {
                function a(b) {
                    this.displayValue = b.displayValue;
                    this.displayValueArgs =
                    b.displayValueArgs;
                    this.style = b.style;
                    this.appliedStyle = b.appliedStyle;
                    this.categoryLabel = b.categoryLabel;
                    this.toolText = b.toolText;
                    this.legendCosmetics = b.legendCosmetics;
                    this.showInLegend = b.showInLegend;
                    this.y = b.y;
                    this.legendColor = b.legendColor;
                    this.shadow = b.shadow;
                    this.smartTextObj = b.smartTextObj;
                    this.color = b.color;
                    this.alpha = b.alpha;
                    this.name = b.name;
                    this.legendItemId = b.legendItemId;
                    this.rawColor = b.rawColor;
                    this.rawAlpha = b.rawAlpha;
                    this.borderColor = b.borderColor;
                    this.borderWidth = b.borderWidth;
                    this.link =
                    b.link;
                    this.isSliced = b.isSliced;
                    this.doNotSlice = b.doNotSlice;
                    this.hoverEffects = b.hoverEffects;
                    this.rolloverProperties = b.rolloverProperties
                }
                var b = this;
                a.upperRadiusFactor = 0;
                a.prototype.getModifiedCurrentValue = function(a) {
                    return a + this.y / 2
                };
                a.prototype.getRatioK = function(a) {
                    var c = b.conf;
                    return a ? a / c.sumValue : 1
                };
                a.prototype.getLowestRadiusFactor = function() {
                    return 1
                };
                return a
            }, drawIndividualPlot: function(a, b) {
                var e = this.conf, c = this.graphics.plotItems, u = a.y, g = a.displayValue, H = e.isSliced, B = this.chart, f =
                this.components.data, r = B.graphics.trackerGroup, y, W = B.components.paper, p=!!a.link, ia = e.distanceAvailed;
                y = a.labelAline;
                var v = a.appliedStyle, l = a.style, k = B.components.legend, B = B.get(L, D).duration, n = this.labelDrawingConfig, l = v && v.color || l && l.color || this._chartLevelAttr.color, H = H ? 1: a.isSliced, l = {
                    text: g,
                    ishot: !0,
                    direction: d,
                    cursor: p ? "pointer": d,
                    x: 0,
                    y: 0,
                    fill: l,
                    "text-anchor": I[y]
                };
                if (null !== u && void 0 !== u && a.shapeArgs)
                    return (y = c[b]) ? (a.plot = y, a.shapeArgs.graphics = y.graphic, a.shapeArgs.animationDuration = B, y.graphic =
                    this.pyramidFunnelShape(a.shapeArgs).attr({
                        fill: a.color,
                        "stroke-width": a.borderWidth,
                        stroke: a.borderColor
                    }), y.graphic.show()) : (a.shapeArgs.graphics = y, a.shapeArgs.animationDuration = B, a.plot = y = c[b] = {
                        graphic: this.pyramidFunnelShape(a.shapeArgs).attr({
                            fill: a.color,
                            "stroke-width": a.borderWidth,
                            stroke: a.borderColor
                        }),
                        trackerObj: W.path(r)
                    }), n[b] = c = {
                        args: l,
                        css: v,
                        point: a
                    }, e.showTooltip ? y.trackerObj.tooltip(a.toolText) : y.trackerObj.tooltip(!1), y.value = u, y.displayValue = g, y.sliced=!!H, y.cursor = p ? "pointer" : d,
                    y.x = a.x, y.index = b, k.configureItems(f[b].legendItemId, {
                        legendClickFN: this.legendClickHandler({
                            datasetStore: this,
                            plotItem: y
                        })
                    }), u = g = {}, a.hoverEffects && (u = {
                        color: a.rawColor,
                        opacity: a.rawAlpha,
                        "stroke-width": a.borderWidth,
                        stroke: a.borderColor
                    }, g = a.rolloverProperties, g = {
                        color: g.color,
                        opacity: g.alpha,
                        "stroke-width": g.borderWidth,
                        stroke: g.borderColor
                    }), f = {
                        datasetStore: this,
                        plotItem: y
                    }, y.trackerObj.unclick(this.slice), !a.doNotSlice && y.trackerObj.click(this.slice, f), y.trackerObj.mouseup(this.plotMouseUp, y),
                    y.trackerObj.hover(this.rolloverResponseSetter(y, g), this.rolloutResponseSetter(y, u)), c.context = f, c.actions = {
                        click: this.slice,
                        hover: [this.rolloverResponseSetter(y, g), this.rolloutResponseSetter(y, u)]
                    }, y.dy = 0, e.noOfGap && (ia && (y._startTranslateY = u = "t0," + ia, y.dy = y.distanceAvailed = ia, y.graphic.attr({
                        transform: u
                    }), c.transform = u), e.slicingGapPosition[a.x] && (e.distanceAvailed -= e.perGapDistance)), y.point = a, y;
                n[b] = {
                    args: l,
                    css: v,
                    point: a
                }
            }, getTooltipMacroStub: function(a) {
                var b = this.conf, d = this.chart.components.numberFormatter,
                c = this.__base__, e;
                e = b.percentOfPrevious ? a.pValue : d.percentValue(a.dataValue / a.prevValue * 100);
                c = c.getTooltipMacroStub(a);
                c.percentValue = b.percentOfPrevious ? d.percentValue(a.dataValue / a.highestValue * 100) : a.pValue;
                c.percentOfPrevValue = e;
                return c
            }
        }, "FunnelPyramidBase"])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-hlineargauge", function() {
        var b = this, k = b.hcLib, d = k.pluckNumber, a = k.pluck, n = k.COMMASTRING, L = k.extend2, D = k.BLANKSTRING, u = k.getValidValue, e = k.parseUnsafeString, C = k.parseTooltext,
        N = k.graphics.convertColor, J = k.POSITION_MIDDLE, K = k.preDefStr, I = K.configStr, G = K.animationObjStr, Z = K.POSITION_TOP, S = K.POSITION_BOTTOM, c = K.POSITION_MIDDLE, F = K.POSITION_START, K = K.POSITION_END, g = k.plotEventHandler, H = b.window, B = void 0 !== H.document.documentElement.ontouchstart, k = Math, f = k.max, r = k.min, y = {
            right: K,
            left: F,
            middle: c,
            start: F,
            end: K,
            center: c,
            undefined: D,
            BLANK: D
        };
        E.register("component", ["dataset", "hlineargauge", {
            pIndex: 2,
            customConfigFn: "_createDatasets",
            init: function(a) {
                this.pointerArr = a;
                this.idMap =
                {};
                this.configure()
            },
            configure: function() {
                var c = this.chart, b = c.jsonData, f = b.chart, g = c.components, l = g.numberFormatter, g = g.colorManager, r = this.config || (this.config = {}), y = this.components || (this.components = {}), m = (b = b.pointers && b.pointers.pointer) && b.length || 1, h, w, q, z, A, x, H, B, t, pa, k, c = c.config.style, F, G, I, K, J, M, L, E, fa, sa, ha, da, X, ba;
                r.valuePadding = d(f.valuepadding, 2);
                r.tooltipSepChar = a(f.tooltipsepchar, n);
                pa = d(f.ticksbelowgauge, f.ticksbelowgraph, 1);
                r.axisPosition = pa ? 3 : 1;
                r.pointerOnOpp = z = Number(!d(f.pointerontop,
                pa, 1));
                r.valueabovepointer = pa = d(f.valueabovepointer, !z, 1);
                r.valueInsideGauge = pa === z ? 1 : 0;
                r.showPointerShadow = d(f.showpointershadow, f.showshadow, 1);
                r.showTooltip = d(f.showtooltip, 1);
                r.textDirection = "1" === f.hasrtltext ? "rtl" : D;
                r.showGaugeLabels = d(f.showgaugelabels, 1);
                r.colorRangeStyle = {
                    fontFamily: c.inCanfontFamily,
                    fontSize: c.inCanfontSize,
                    lineHeight: c.inCanLineHeight,
                    color: c.inCancolor.replace(/^#?/, "#")
                };
                r.showValue = d(f.showvalue, 1);
                r.editMode = d(f.editmode, 0);
                r.pointerSides = c = d(f.pointersides, 3);
                r.pointerBorderThickness =
                ba = d(f.pointerborderthickness);
                r.showHoverEffect = pa = d(f.showhovereffect, f.plothovereffect);
                r.upperLimit = d(f.upperlimit);
                r.lowerLimit = d(f.lowerlimit);
                r.startAngle = 90 * {
                    top: 1,
                    right: 0,
                    left: 2,
                    bottom: 3
                }
                [z ? Z: S];
                B = y.data || (y.data = []);
                for (z = 0; z < m; z++)
                    w = B[z] || (y.data[z] = {}), w = w.config || (w.config = {}), h = b && b[z] || {}, w.itemValue = q = l.getCleanValue(h.value), w.formatedVal = x = l.dataLabels(q), w.setDisplayValue = H = u(e(h.displayvalue)), w.setToolText = A = u(e(h.tooltext)), w.id = a(h.id, "pointer_" + z), w.showHoverEffect = q = d(h.showhovereffect,
                    pa), w.showBorder = G = d(h.showborder, f.showplotborder, 1), w.borderWidth = E = G ? d(h.borderthickness, ba) : 0, (w.showValue = t = d(h.showvalue, r.showValue)) ? void 0 !== H ? (w.displayValue = H, w.isLabelString=!0) : w.displayValue = u(x, D) : w.displayValue = D, w.sides = H = d(h.sides, c), 3 > H && (w.sides = 3), w.radius = H = d(h.radius, f.pointerradius, 10), void 0 !== A ? (w.toolText = C(A, [1, 2], {
                    formattedValue: x
                }, h, f), w.isTooltextString=!0) : w.toolText = null === x?!1 : x, w.tempToolText = w.toolText, w.bgAlpha = t = d(h.alpha, h.bgalpha, f.pointerbgalpha, 100), w.bgColor =
                k = a(h.color, h.bgcolor, f.pointerbgcolor, f.pointercolor, g.getColor("pointerBgColor")), w.fillColor = X = N(k, t), w.showBorder = d(h.showborder, f.showplotborder, 1), w.borderAlpha = x = d(h.borderalpha, f.pointerborderalpha, 100), w.borderColor = A = a(h.bordercolor, f.pointerbordercolor, g.getColor("pointerBorderColor")), w.pointerBorderColor = F = N(A, x), w.dataLink = u(h.link), w.editMode = d(h.editmode, r.editMode), 0 !== q && (q || h.bghovercolor || f.pointerbghovercolor || f.plotfillhovercolor || h.bghoveralpha || f.pointerbghoveralpha || f.plotfillhoveralpha ||
                0 === h.bghoveralpha || 0 === f.pointerbghoveralpha || h.showborderonhover || f.showborderonhover || 0 === h.showborderonhover || 0 === f.showborderonhover || h.borderhoverthickness || f.pointerborderhoverthickness || 0 === h.borderhoverthickness || 0 === f.pointerborderhoverthickness || h.borderhovercolor || f.pointerborderhovercolor || h.borderhoveralpha || f.pointerborderhoveralpha || 0 === h.borderhoveralpha || 0 === f.pointerborderhoveralpha || h.hoverradius || f.pointerhoverradius || 0 === h.hoverradius || 0 === f.pointerhoverradius) && (q=!0, K = a(h.bghovercolor,
                f.pointerbghovercolor, f.plotfillhovercolor, "{dark-10}"), I = d(h.bghoveralpha, f.pointerbghoveralpha, f.plotfillhoveralpha), fa = d(h.showborderonhover, f.showborderonhover), void 0 === fa && (fa = h.borderhoverthickness || 0 === h.borderhoverthickness || h.borderhovercolor || h.borderhoveralpha || 0 === h.borderhoveralpha ? 1 : G), J = a(h.borderhovercolor, f.pointerborderhovercolor, "{dark-10}"), G = d(h.borderhoveralpha, f.pointerborderhoveralpha), M = fa ? d(h.borderhoverthickness, f.pointerborderhoverthickness, E || 1) : 0, L = d(h.hoverradius, f.pointerhoverradius,
                H + 2), h=!!d(h.showhoveranimation, f.showhoveranimation, 1), w.hoverAttr = fa = {}, w.outAttr = sa = {}, E !== M && (fa["stroke-width"] = M, sa["stroke-width"] = E), sa.fill = X, K = (E = /\{/.test(K)) ? g.parseColorMix(k, K)[0] : K, fa.fill = N(K, d(I, t)), M && (sa.stroke = F, E = /\{/.test(J), fa.stroke = N(E ? g.parseColorMix(A, J)[0] : J, d(G, x))), L && (h ? (ha = {
                    r: L
                }, da = {
                    r: H
                }) : (fa.r = L, sa.r = H))), w.rolloverProperties = {
                    enabled: q,
                    hoverAttr: fa,
                    hoverAnimAttr: ha,
                    outAttr: sa,
                    outAnimAttr: da
                }
            },
            _manageSpace: function(a) {
                var c = this.chart, b = this.components, f = c.config,
                l = this.config, g = c.linkedItems.smartLabel, r = f.canvasWidth, m, h = f.dataLabelStyle, e = f.style.trendStyle, q = c.jsonData, z = q.trendpoints && q.trendpoints.point, A = d(parseInt(h.lineHeight, 10), 12), q = l.valuePadding, x = l.valueInsideGauge, y, H = y = 0, t = 0, u = 0, B = 0, k = 0, n = 0, F = c.components.scale.config.spaceTaken, C = 0, G = this.pointerArr && this.pointerArr.pointer && this.pointerArr.pointer.length, I = l.pointerOnOpp, b = b.data;
                z && c._configueTrendPoints();
                g.useEllipsesOnOverflow(f.useEllipsesWhenOverflow);
                for (g.setStyle(h); C < G; C += 1)
                    c =
                    b[C].config, y = q + c.radius * (3 >= c.sides ? .5 : c.sides%2 ? 1.1 - 1 / c.sides : 1), u = Math.max(u, y), c.showValue && c.displayValue !== D && (c.isLabelString ? (m = g.getSmartText(c.displayValue, r, a - u), c.displayValue = m.text, m.tooltext && (c.originalText = m.tooltext)) : m = g.getOriSize(c.displayValue), c.displayValue === D && (m = {
                        height: A
                    })), y = m && 0 < m.height&&!x ? m.height + y : y, y > a && (y = a), I ? (3 === l.axisPosition && (k = Math.max(F.bottom, k), y = Math.max(F.bottom, y)), H = Math.max(H, y)) : (1 === l.axisPosition && (n = Math.max(F.top, n), y = Math.max(F.top, y)), t = Math.max(y,
                    t)), l.align = J;
                l.currentValuePadding = u;
                g.setStyle(e);
                if (z) {
                    f = f.trendPointConfig;
                    C = 0;
                    for (G = f.length; C < G; C += 1)(r = f[C]) 
                        && r.displayValue !== D && (e = q + .5 * r.markerRadius, B = Math.max(e, B), m = g.getOriSize(r.displayValue), 0 < m.height && (y = m.height + e), y > a && (y = a), r.showOnTop ? (1 === l.axisPosition && (n = Math.max(F.top, n), y = Math.max(F.top, y)), t = Math.max(t, y)) : (3 === l.axisPosition && (k = Math.max(F.bottom, k), y = Math.max(F.bottom, y)), H = Math.max(y, H)));
                    l.currentTrendPadding = B
                }
                return {
                    top: t - n,
                    bottom: H - k
                }
            },
            draw: function(a, c) {
                var f =
                this, r = f.idMap, l = f.chart, e = l.components, y = l.config, m = l.graphics, h = m.datalabelsGroup, w = m.tempGroup, q = e.paper, z = e.scale, A = f.pointerArr && f.pointerArr.pointer, e = y.canvasWidth, y = y.canvasHeight, x = f.config, y = x.pointerOnOpp ? y: 0, u = x.showPointerShadow, k = x.showTooltip, t = z.config.axisRange.min, n = z.config.axisRange.max, F = (n - t) / e, A = A && A.length || 1, C, K, Q, J;
                C = z.config.prevAxisMinMax || (z.config.prevAxisMinMax = {});
                var N = f.components.data, M = l.get(I, G), L = M.animObj, Z = M.dummyObj, fa = M.animType, M = (a || M).duration, S = {
                    pageX: 0,
                    pageY: 0
                }, ha = function(a, c) {
                    var b = c[0];
                    !1 !== this.editMode && (K = l._getDataJSON(), this.dragStartX = b)
                }, da = function() {
                    var a = this.config, c, h = l.chartInstance;
                    if (!1 !== this.config.editMode) {
                        (c = h && h.jsVars) && (c._rtLastUpdatedData = l._getDataJSON());
                        b.raiseEvent("RealTimeUpdateComplete", {
                            data: "&value=" + a.updatedValStr,
                            updateObject: {
                                values: [a.updatedValStr]
                            },
                            prevData: K.values,
                            source: "editMode",
                            url: null
                        }, h);
                        try {
                            H.FC_ChartUpdated && H.FC_ChartUpdated(h.id)
                        } catch (q) {
                            setTimeout(function() {
                                throw q;
                            }, 1)
                        }
                        x.showTooltip ? ba.tooltip(a.toolText) :
                        ba.tooltip(D)
                    }
                }, X = function(a, c) {
                    var b = c[2], h = B && (B && a.sourceEvent && a.sourceEvent.touches && a.sourceEvent.touches[0] || a) || S, q = z.getLimit(), p = q.min, q = q.max, g = d(this.itemValue, p), r = g - (this.dragStartX - b) * F, l = 0, m = [], x = [];
                    if (!1 !== this.editMode) {
                        this.drag=!0;
                        for (r < p ? r = p : r > q && (r = q); l < this.index; l += 1)
                            m.push(D), x.push(D);
                        m.push({
                            value: r
                        });
                        x.push(r);
                        g !== r && f.updateData({
                            data: m
                        }, {
                            duration: 0
                        }, !0) && (this.updatedValStr = x.join("|"), this.dragStartX = b || a.pageX || h.pageX)
                    }
                }, ba, U, V, aa, E, oa, O = f.pool;
                w || (w = m.tempGroup = q.group("tempGroup",
                h).trackTooltip(!0));
                m = function(a) {
                    var c = this.data("eventArgs").index, c = N[c].config;
                    !0 === c.drag ? c.drag=!1 : g.call(this, l, a)
                };
                h = function(a) {
                    var c = this.data("rolloverProperties");
                    c.enabled && (this.attr(c.hoverAttr), c.hoverAnimAttr && this.animate(c.hoverAnimAttr, 100, "easeIn"));
                    g.call(this, l, a, "DataPlotRollOver")
                };
                oa = function(a) {
                    var c = this.data("rolloverProperties");
                    c.enabled && (this.attr(c.outAttr), c.outAnimAttr && this.animate(c.outAnimAttr, 100, "easeIn"));
                    g.call(this, l, a, "DataPlotRollOut")
                };
                Q = z.config.axisRange;
                if (C.min !== Q.min || C.max !== Q.max)
                    l._drawAxis(), l._drawCanvas();
                C && (C.min = Q.min, C.max = Q.max);
                for (; A--;)
                    C = N[A].config, U = N[A].graphics || (N[A].graphics = {}), aa = C.rolloverProperties || {}, J = x.startAngle, r[C.id] = {
                        index: A,
                        config: C
                    }, J += .2, Q = u ? {
                        opacity: Math.max(C.bgAlpha, C.borderAlpha) / 100
                    } : !1, V = C.dataLink, E = {
                        index: A,
                        link: V,
                        value: C.itemValue,
                        displayValue: C.displayValue,
                        toolText: C.toolText
                    }, (ba = U.pointer) || (O && O.pointer[0] ? (ba = U.pointer = O.pointer[0], O.pointer.splice(0, 1)) : ba = U.pointer = q.polypath(w).click(m).hover(h,
                    oa), ba.attr({
                        polypath: [C.sides, 0, y || 0, C.radius, J, 0, w]
                    }), ba.show(), ba.drag(X, ha, da, C, C, N[A])), c || (ba.attr({
                        fill: C.fillColor,
                        stroke: C.pointerBorderColor,
                        ishot: !0,
                        "stroke-width": C.borderWidth
                    }).shadow(!!Q, Q && Q.opacity).data("eventArgs", E).data("rolloverProperties", aa), V || C.editMode ? ba.css({
                        cursor: "pointer",
                        _cursor: "hand"
                    }) : ba.css({
                        cursor: D,
                        _cursor: D
                    }), C._startAngle = J, C.editMode ? (C.index = A, C.editMode=!0) : C.editMode=!1, ba.attr({
                        ishot: !0
                    })), k ? ba.tooltip(C.toolText, null, null, !0) : ba.tooltip(D), ba.animateWith(Z,
                    L, {
                        polypath: [C.sides, e * (d(C.itemValue, t) - t) / (n - t), y || 0, C.radius, J, 0],
                        r: C.radius
                    }, M, fa);
                c ? f.drawPointerValues(a) : f._drawWidgetLabel(a);
                f.removeDataArr && f.remove()
            },
            removeData: function(a) {
                var c = this.components.data;
                this.removeDataArr || (this.removeDataArr = []);
                this.removeDataArr = this.removeDataArr.concat(c.splice(0, a))
            },
            remove: function() {
                var a = this.removeDataArr, c = a.length, b, f, r, g, d = this.pool || (this.pool = {
                    pointer: [],
                    pointerValue: []
                });
                for (g = 0; g < c; g++)
                    b = a[g].graphics, r = b.pointer, f = b.pointerValue, d.pointer[g] =
                    b.pointer, d.pointerValue[g] = b.pointerValue, f.hide(), r.hide(), r.undrag(), r.shadow(!1);
                delete this.removeDataArr
            },
            _drawWidgetLabel: function(b) {
                var f = this.chart, r = f.config, g = f.components, l = g.numberFormatter, e = g.scale, H = g.paper, m = f.graphics.datalabelsGroup, h = e.config.axisRange.min, w = e.config.axisRange.max, q = this.config, e = q.textDirection, z = g.colorRange && g.colorRange.getColorRangeArr(h, w), g = q.colorRangeStyle || {}, A = q.showvalue, x, u = f.jsonData.trendpoints && f.jsonData.trendpoints.point, B = r.trendPointConfig,
                t = r.canvasWidth, k = r.canvasHeight, n = r.marginRight, F = q.pointerOnOpp;
                x = q.valueInsideGauge;
                var C = q.showGaugeLabels, K = r.dataLabelStyle, J = f.get(I, G), L = J.animObj, M = J.dummyObj, Z = J.animType, S = J.duration, fa, E, ha, da, X, ba, U, V;
                U=!1;
                var f = f.linkedItems.smartLabel, aa, la, oa = d(parseInt(K.fontHeight, 10), parseInt(K.lineHeight, 10), 12), O = q.currentValuePadding + .5 * oa, J = q.currentTrendPadding + .5 * oa, P, q = q.currentTrendPadding;
                ha = {
                    fontFamily: K.fontFamily,
                    fontSize: K.fontSize,
                    lineHeight: K.lineHeight,
                    fontWeight: K.fontWeight,
                    fontStyle: K.fontStyle
                };
                var Y = this.pointerArr && this.pointerArr.pointer, T, K = this.components, qa = K.data, wa = K.dataLabels || (K.dataLabels = []), ja = K.trendLabels || (K.trendLabels = []);
                m.transform(["T", r.canvasLeft, r.canvasTop]);
                O = x === F ? O - oa / 4 : O + oa / 4;
                f.useEllipsesOnOverflow(r.useEllipsesWhenOverflow);
                f.setStyle(ha);
                la = f.getOriSize("W...").width;
                g.fontWeight = "normal";
                ha = this.getPointerLabelXY = function(a, b, q, f, r) {
                    b = q ? b ? k - f - O : k + O : b ? O : - (O + f);
                    P = (a - h) * t / (w - h);
                    P + r > t + n && (P = t - r + n);
                    return {
                        x: P,
                        y: b,
                        align: c
                    }
                };
                da = function(a, c) {
                    return {
                        x: (a - h + (c - a) /
                        2) * t / (w - h),
                        y: k / 2,
                        width: (c - a) * t / (w - h),
                        height: k
                    }
                };
                if (Y && Y.length)
                    for (r = Y.length; r--;)
                        if (T = qa[r].config, 0 !== A && T.displayValue !== D && (aa = f.getOriSize(T.displayValue), T.setWidth && (aa = f.getSmartText(T.displayValue, T.setWidth, aa.height, !0)), X = this.getPointerLabelXY(T.itemValue, x, F, aa.height / 2, aa.width / 2), T.isLabelString)) {
                            U=!1;
                            for (V = 1; !U;) {
                                ba = Y[r + V];
                                if (!ba)
                                    break;
                                    ba.isLabelString ? U=!0 : V += 1
                            }
                            ba && (U = f.getOriSize(ba.displayValue), fa = ha(ba.y, x, F, U.height / 2), V = fa.x - U.width / 2 - (X.x + aa.width / 2), fa = fa.x - X.x, 0 > V && (E =
                            aa.width + V, E > fa && (T.setWidth = E = fa), E > la ? (X = T.setWidth && T.setWidth <= E ? f.getSmartText(T.displayValue, T.setWidth, aa.height, !0) : f.getSmartText(T.displayValue, E, aa.height, !0), T.displayValue = X.text, X.tooltext && (T.originalText = X.tooltext)) : (X = f.getSmartText(T.displayValue, la, aa.height, !0), T.displayValue = X.text, X.tooltext && (T.originalText = X.tooltext), V = 2 * V + la - 4), T.setWidth = null, E = U.width + V - 4, ba.setWidth = E > fa ? fa : E > la ? E : la));
                            T.setWidth && (X = f.getSmartText(T.displayValue, T.setWidth, aa.height, !0), T.displayValue =
                            X.text, X.tooltext && (T.originalText = X.tooltext), T.setWidth = null)
                        }
                this.drawPointerValues(b);
                f.setStyle(g);
                if (z && C)
                    for (r = 0, b = z.length; r < b; r += 1)
                        A = wa[r] || (K.dataLabels[r] = {}), A = A.graphics || (A.graphics = {}), x = z[r], F = a(x.label, x.name), X = da(x.minvalue, x.maxvalue), aa = X.width - 4 > la && X.height - 4 > oa ? f.getSmartText(F, X.width - 4, X.height - 4) : f.getSmartText(F, X.width, X.height), F = {
                            "text-anchor": c,
                            "vertical-align": c,
                            x: X.x,
                            y: X.y,
                            direction: e,
                            fill: g.color,
                            text: aa.text
                        }, (x = A.value) ? (x.show(), x.animateWith(M, L, F, S, Z).css(g).tooltip(aa.tooltext)) :
                        x = A.value = H.text(F, g, m);
                else 
                    r = 0;
                for (; A = wa && wa[r++];)
                    A.graphics.value.hide();
                if (u)
                    for (r = 0, b = B.length; r < b; r += 1) {
                        z = ja[r] || (K.trendLabels[r] = {});
                        A = z.graphics || (z.graphics = {});
                        z = B[r];
                        z.displayValue = a(z.displayValue, l.dataLabels(z.startValue));
                        f.setStyle(z.style);
                        oa = f.getOriSize("Wg").height;
                        aa = f.getOriSize(z.displayValue);
                        X = ha(z.startValue, 0, !z.showOnTop);
                        z.setWidth && (aa = f.getSmartText(z.displayValue, z.setWidth, aa.height, !0));
                        U=!1;
                        for (V = 1; !U;) {
                            ba = u[r + V];
                            if (!ba)
                                break;
                                ba.showOnTop === z.showOnTop ? U=!0 : V +=
                                1
                        }
                        ba && (U = f.getOriSize(ba.displayValue), fa = ha(ba.startValue, 0, !ba.showOnTop), V = fa.x - U.width / 2 - (X.x + aa.width / 2), 0 > V && (fa = fa.x - X.x, E = aa.width + V, E > fa && (z.setWidth = E = fa), E > la ? (aa = z.setWidth && z.setWidth <= E ? f.getSmartText(z.displayValue, z.setWidth, aa.height, !0) : f.getSmartText(z.displayValue, aa.width + V - 4, aa.height, !0), z.displayValue = aa.text, aa.tooltext && (z.originalText = aa.tooltext)) : (aa = f.getSmartText(z.displayValue, la, aa.height, !0), z.displayValue = aa.text, aa.tooltext && (z.originalText = aa.tooltext), V = 2 * V + la -
                        4), z.setWidth = null, E = U.width + V - 4, ba.setWidth = E > fa ? fa : E > la ? E : la));
                        z.setWidth && (aa = f.getSmartText(z.displayValue, z.setWidth, aa.height, !0), z.displayValue = aa.text, aa.tooltext && (z.originalText = aa.tooltext), z.setWidth = null);
                        M = z.showOnTop?-(q + aa.height / 2) : k + J;
                        L = z.isTrendZone ? da(z.startValue, z.endValue).x : X.x;
                        A.value || (A.value = H.text(m));
                        A.value.attr({
                            x: L,
                            y: M,
                            text: z.displayValue,
                            "text-anchor": y[X.align],
                            fill: N(z.textColor || g.color),
                            "font-weight": "normal",
                            direction: e,
                            title: z.originalText || D
                        });
                        A.value.show()
                    } else 
                        r =
                        0;
                for (; z = ja && ja[r++];)
                    z.graphics.value.hide()
            },
            drawPointerValues: function(a) {
                var c = this.chart, b = c.graphics.datalabelsGroup, f = c.components.paper, r = this.components.data, g = this.config, d = g.pointerOnOpp, m = g.valueInsideGauge, g = g.textDirection, h, e = c.linkedItems.smartLabel, q = this.pointerArr && this.pointerArr.pointer, z = c.config.dataLabelStyle, q = q && q.length, A, x = c.get(I, G), H = x.animObj, u = x.dummyObj, t = x.animType;
                a = (a || x).duration;
                var B = c.config, c = B.marginLeft, x = {
                    fontFamily: z.fontFamily,
                    fontSize: z.fontSize,
                    lineHeight: z.lineHeight,
                    fontWeight: z.fontWeight,
                    fontStyle: z.fontStyle
                }, k, n, F = this.pool, C;
                for (e.useEllipsesOnOverflow(B.useEllipsesWhenOverflow); q--;)
                    C=!1, B = r[q].graphics, A = r[q].config, k = A.displayValue, h = A.showValue, 0 !== h && k !== D ? (n = e.getOriSize(k), h = n.width / 2, n = this.getPointerLabelXY(A.itemValue, m, d, n.height / 2, h), k = {
                        "text-anchor": y[n.align],
                        title: A.originalText || D,
                        text: k,
                        fill: z.color,
                        direction: g,
                        "text-bound": [z.backgroundColor, z.borderColor, z.borderThickness, z.borderPadding, z.borderRadius, z.borderDash]
                    }, (A = B.pointerValue) ||
                    (F && F.pointerValue[0] ? (A = B.pointerValue = F.pointerValue[0], F.pointerValue.splice(0, 1)) : (k.x = 0, k.y = n.y, C=!0, A = B.pointerValue = f.text(k, x, b))), C || (A.attr(k).css(x), A.show()), h > c + n.x && (n.x = h - c), A.animateWith(u, H, {
                        x: n.x,
                        y: n.y
                    }, a, t)) : B.pointerValue && B.pointerValue.hide()
            },
            getDataLimits: function() {
                var a = this.config, c = this.chart.jsonData, b = this.components.data || this.pointerArr && this.pointerArr.pointer || c.dials && c.dials.dial, c = (c = c.colorrange) && c.color, g = b && b.length, d, e, y, m = a.upperLimit, h = a.lowerLimit, w, q,
                z = 0;
                q = 0;
                var A = void 0 === a.gaugeMax?-Infinity : a.gaugeMax, x = void 0 === a.gaugeMin ? Infinity : a.gaugeMin;
                for (d = 0; d < g; d++)
                    w = b[d].value || b[d].config && b[d].config.itemValue, w !== D && void 0 !== w && (y = A = f(A, Number(w)), e = x = r(x, Number(w)));
                g = c && c.length;
                for (d = 0; d < g; d++)
                    q = Number(c[d].maxvalue), b = Number(c[d].minvalue), m && q > m && (q = m), h && b < h && (b = h), z = A < q ? A : !1, A = f(A, q), q = x > b ? x : !1, x = r(x, b);
                a.gaugeMax = A;
                a.gaugeMin = x;
                !1 !== z && (a.gaugeMax = z);
                !1 !== q && (a.gaugeMin = q);
                return {
                    forceMin: e !== x,
                    forceMax: y !== A,
                    max: A,
                    min: x
                }
            },
            updateData: function(a,
            c, b) {
                if (a === this.lastUpdatedObj)
                    return !1;
                var f = this.chart, r = f.components.numberFormatter, g = this.components.data, d, m = this.components.data, m = m && m.length || 0, h, y, q = null, z = [], A, x, H;
                a = a.data;
                x = c || f.get(I, G);
                if (m) {
                    for (; m--;)
                        if (h = {}, H = {}, y = g[m].config, d = a[m])
                            A = d.value, f = d.tooltext, c = d.label, d = d.showlabel, void 0 !== A && A !== D ? (h.value = H.value = A, q = H.displayvalue = H.tooltext = r.dataLabels(H.value), H.hasNewData=!0) : H.value = y.formatedVal, c && (H.displayvalue = c, H.hasNewData=!0), "0" == d && (H.displayvalue = D, H.hasNewData =
                    !0), f && (f = u(e(f)), H.hasNewData=!0), H.hasNewData && (z[m] = H, L(y, {
                        itemValue: H.value,
                        displayValue: "0" !== d ? H.displayvalue || D: D,
                        toolText: void 0 !== f ? C(f,
                        [1,
                        2],
                        {
                            formattedValue: q
                        }, h) : y.setToolText ? y.tempToolText : q
                    }));
                    z.length && (this.lastUpdatedObj = a, b && this.draw(x, !0));
                    return !!z.length
                }
            }
        }
        ])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-bullet", function() {
        var b = this.hcLib, k = b.preDefStr, d = b.BLANKSTRING, a = b.pluck, n = b.getValidValue, L = b.COMMASTRING, D = b.plotEventHandler, u = k.showHoverEffectStr, e =
        k.configStr, C = k.animationObjStr, N = b.getColorCodeString, J = k.ROUND, K = k.miterStr, I = b.HUNDREDSTRING, G = Math, Z = G.max, S = G.min, c = G.abs, F = k.colors.FFFFFF, g = b.graphics.convertColor, H = b.toRaphaelColor, B = b.COMMASPACE, f = b.getFirstValue, r = b.graphics.getDarkColor, y = this.window, G = void 0 !== y.document.documentElement.ontouchstart, W = b.CLICK_THRESHOLD_PIXELS, p = "rgba(192,192,192," + (/msie/i.test(y.navigator.userAgent)&&!y.opera ? .002 : 1E-6) + ")", ia = {
            "true": void 0,
            "false": "crisp"
        }, v = k.POSITION_START, l = k.POSITION_TOP, ra = k.POSITION_MIDDLE,
        Da = k.PLOTFILLCOLOR_STR, k = b.TOUCH_THRESHOLD_PIXELS, m = G ? k : W, h = b.pluckNumber, w = b.schedular;
        E.register("component", ["dataset", "bullet", {
            init: function(a) {
                var c = this.chart, b = c.components;
                if (!a)
                    return !1;
                this.JSONData = a;
                this.yAxis = b.scale;
                this.chartGraphics = c.chartGraphics;
                this.components = {};
                this.graphics = {};
                this.visible = 1 === h(this.JSONData.visible, !Number(this.JSONData.initiallyhidden), 1);
                this.configure();
                !1 !== c.hasLegend && this._addLegend()
            },
            configure: function() {
                var c = this.chart, f = this.config, r = this.JSONData,
                d = c.jsonData.chart, l = c.components.colorManager, m, p = f.plotColor = l.getColor(Da), e = h(r.dashed, d.plotborderdashed);
                h(d.useplotgradientcolor, 1);
                var v, y, w, H = b.getDashStyle, u = this.components.data, k = c.isBar, B = c.is3D, n = c.isStacked;
                f.targetCapStyle = v = a(d.targetcapstyle, J).toLowerCase();
                "butt" !== v && v !== J && "square" !== v && "inherit" !== v && (f.targetCapStyle = J);
                f.upperLimit = h(d.upperlimit);
                f.lowerLimit = h(d.lowerlimit);
                f.initAnimation=!0;
                m = f.showplotborder = h(d.showplotborder, 0);
                f.plotDashLen = v = h(d.plotborderdashlen,
                5);
                f.plotDashGap = y = h(d.plotborderdashgap, 4);
                f.plotfillAngle = h(360 - d.plotfillangle, k ? 180 : 90);
                f.plotFillAlpha = w = a(r.alpha, d.plotfillalpha, I);
                f.plotColor = a(d.plotfillcolor, p);
                f.isRoundEdges = p = h(d.useroundedges, 0);
                f.plotRadius = h(d.useRoundEdges, f.isRoundEdges ? 1 : 0);
                f.plotFillRatio = a(r.ratio, d.plotfillratio);
                f.plotgradientcolor = b.getDefinedColor(d.plotgradientcolor, l.getColor("plotGradientColor"));
                f.showPlotBorderOnHover = h(d.showplotborderonhover, 0);
                f.plotBorderAlpha = a(d.plotborderalpha, w, I);
                f.plotBorderColor =
                a(d.plotbordercolor, B ? F : l.getColor("plotBorderColor"));
                f.plotBorderThickness = m = m ? h(d.plotborderthickness, 0) : 0;
                f.plotBorderDashStyle = e ? H(v, y, m) : "none";
                f.showValue = h(r.showvalue, d.showvalue, 1);
                f.valuePadding = h(d.valuepadding, 2);
                f.showShadow = p || B ? h(d.showshadow, 1) : h(d.showshadow, l.getColor("showShadow"));
                f.showHoverEffect = h(d.plothovereffect, d.showhovereffect, 0);
                f.showTooltip = h(d.showtooltip, 1);
                f.stack100Percent = c = h(c.stack100percent, d.stack100percent, 0);
                f.definedGroupPadding = Z(h(d.plotspacepercent),
                0);
                f.plotSpacePercent = Z(h(d.plotspacepercent, 20)%100, 0);
                f.maxColWidth = h(k ? d.maxbarheight : d.maxcolwidth, 50);
                f.showPercentValues = h(d.showpercentvalues, n && c ? 1 : 0);
                f.showPercentInToolTip = h(d.showpercentintooltip, n && c ? 1 : 0);
                f.plotPaddingPercent = h(d.plotpaddingpercent);
                f.rotateValues = h(d.rotatevalues) ? 270 : 0;
                f.placeValuesInside = h(d.placevaluesinside, 0);
                f.use3DLighting = h(d.use3dlighting, 1);
                u || (this.components.data = []);
                f.plotAsDot = u = h(d.plotasdot, 0);
                f.plotFillPercent = h(d.plotfillpercent, u ? 25 : 40);
                f.targetFillPercent =
                h(d.targetfillpercent, 60);
                f.targetThickness = h(d.targetthickness, 3);
                u = f.targetalpha = h(d.targetalpha, 100);
                f.targetColor = g(a(d.targetcolor, l.getColor(Da)), u);
                this._setConfigure()
            },
            _setConfigure: function(c) {
                var l = this.chart, m = this.config, p = this.JSONData, e = c || p.data, v = e && e.length, t = l.config.categories, t = t && t.length, v = c && c.data.length || S(t, v), y = l.jsonData.chart, t = l.components.colorManager, w = m.showplotborder, u = m.showPlotBorderOnHover, k = m.plotColor, F = b.parseUnsafeString, C = F(a(y.tooltipsepchar, B)), W = h(y.seriesnameintooltip,
                1), D = b.parseTooltext, G, I, ia, K, ra, J, N = m.plotBorderThickness, L = m.isRoundEdges, U = m.showHoverEffect, V = m.plotFillAngle, Da, E = m.plotBorderAlpha, oa, O = m.plotBorderDashStyle, P, Y, T, qa, wa, ja, ka, Fa, Ia, ea, va, ga, Ha = b.getDashStyle, Ca = this.components.data, Ja = l.isBar, Na = l.is3D, xa =- Infinity, ca = Infinity, Ga = l.components.numberFormatter, Oa = function(a) {
                    m.showTooltip ? null === G ? a=!1 : void 0 !== a ? (K = [1, 2, 3, 4, 5, 6, 7, 26, 27], I = {
                        formattedValue : G, targetValue : T.target, targetDataValue : T.toolTipValueTarget
                    }, a = D(a, K, I, P, y, p)) : (W && (ia =
                    f(p && p.seriesname)), a = ia ? ia + C : d, a += T.label ? T.label + C : d) : a=!1;
                    return a
                };
                Ca || (Ca = this.components.data = []);
                for (l = 0;
                l < v;
                l++)P = c ? c && c.data[l]: e[l],
                T = (k = Ca[l]) && k.config,
                k || (k = Ca[l] = {}), k.config || (T = Ca[l].config = {}), T.showValue = h(P.showvalue, m.showValues), T.valuePadding = h(y.valuepadding, 2), T.setValue = Y = Ga.getCleanValue(P.value), T.target = k = Ga.getCleanValue(P.target), T.setLink = a(P.link), T.toolTipValue = oa = Ga.dataLabels(Y), T.toolTipValueTarget = Ga.dataLabels(k), T.setDisplayValue = qa = F(P.displayvalue), T.displayValue =
                a(P.label, qa, oa), oa = h(P.dashed), qa = h(P.dashlen, void 0), wa = J = h(P.dashgap, J), xa = Z(xa, Y, k), ca = S(ca, Y, k), T.plotBorderDashStyle = oa = 1 === oa ? Ha(qa, wa, N) : 0 === oa ? "none" : O, k = a(P.color, m.plotColor), Da = a(P.alpha, m.plotFillAlpha), 0 > Y&&!L && (ra = V, V = Ja ? 180 - V : 360 - V), T.colorArr = Y = b.graphics.getColumnColor(k, Da, void 0, V, L, m.plotBorderColor, E.toString(), Ja ? 1 : 0, Na?!0 : !1), 0 !== U && (qa = a(P.hovercolor, p.hovercolor, y.plotfillhovercolor, y.columnhovercolor, k), wa = a(P.hoveralpha, p.hoveralpha, y.plotfillhoveralpha, y.columnhoveralpha,
                Da), ja = a(P.hovergradientcolor, p.hovergradientcolor, y.plothovergradientcolor, m.plotgradientcolor), !ja && (ja = d), ja = a(P.hoverratio, p.hoverratio, y.plothoverratio, void 0), ka = h(360 - P.hoverangle, 360 - p.hoverangle, 360 - y.plothoverangle, V), Fa = a(P.borderhovercolor, p.borderhovercolor, y.plotborderhovercolor, y.plotfillhovercolor, m.plotBorderColor), Ia = a(P.borderhoveralpha, p.borderhoveralpha, y.plotborderhoveralpha, E, Da), Da = h(P.borderhoverthickness, p.borderhoverthickness, y.plotborderhoverthickness, N), ea = h(P.borderhoverdashed,
                p.borderhoverdashed, y.plotborderhoverdashed), va = h(P.borderhoverdashgap, p.borderhoverdashgap, y.plotborderhoverdashgap, void 0), ga = h(P.borderhoverdashlen, p.borderhoverdashlen, y.plotborderhoverdashlen, J), ea = ea ? Ha(ga, va, Da) : oa, 1 == U && qa === k && (qa = r(qa, 90)), qa = b.graphics.getColumnColor(qa, wa, ja, ka, L, Fa, Ia.toString(), Ja ? 1 : 0, !1), T.setPlotRolloutAttr = {
                    fill: Na ? [H(Y[0]), !m.use3DLighting]: H(Y[0]),
                    stroke: w && H(Y[1]),
                    "stroke-width": N,
                    "stroke-dasharray": oa
                }, T.setPlotRolloverAttr = {
                    fill: Na ? [H(qa[0]), !m.use3DLighting]:
                    H(qa[0]),
                    stroke: H(qa[1]),
                    "stroke-width": u ? Da || 1: Da,
                    "stroke-dasharray": ea
                }), 0 !== U && (U || y.targethovercolor || y.targethoveralpha || 0 === y.targethoveralpha || y.targethoverthickness || 0 === y.targethoverthickness) && (U=!0, oa = {}, Y = {}, ja = h(y.targethoverthickness, m.targetThickness + 2), m.targetThickness !== ja && (oa["stroke-width"] = ja, Y["stroke-width"] = m.targetThickness), qa = a(y.targethovercolor, "{dark-10}"), wa = h(y.targethoveralpha, m.targetalpha), ja && (Y.stroke = m.targetColor, ja = /\{/.test(qa), oa.stroke = g(ja ? t.parseColorMix(a(y.targetcolor,
                k), qa)[0] : qa, wa)), T.tagetHoverAttr = oa, T.targetOutAttr = Y), G = T.toolTipValue, k = n(F(a(P.tooltext, p.plottooltext, y.plottooltext))), T.toolText = Oa(k), T.setTooltext = T.toolText, ra && (V = ra), k = n(F(a(P.tooltexttarget, p.targettooltext, y.targettooltext))), T.toolTextTarget = Oa(k);
                m.maxValue = xa;
                m.minValue = ca
            }, _manageSpace: function(a) {
                var c = this.config, b = this.components.data, f = this.chart, r = f.components.caption.config, g = f.config, f = f.linkedItems.smartLabel, m = g.dataLabelStyle, l = h(parseInt(m.lineHeight, 10), 12), p = c.valuePadding,
                e = 0, y = 0, b = (b = b[y]) && b.config;
                f.useEllipsesOnOverflow(g.useEllipsesWhenOverflow);
                for (f.setStyle(m); 1 > y; y += 1)
                    c.showValue && (g = f.getOriSize(b.toolTipValue), b.toolTipValue === d && (g = {
                        height: l
                    }), 0 < g.height && (e = g.height + p), e > a && (e = a));
                r.widgetValueHeight = e;
                c.heightUsed = e;
                return {
                    top: 0,
                    bottom: e
                }
            }, _manageSpaceHorizontal: function(a) {
                var c = this.config, b = this.components.data, f = this.chart, r = f.config, f = f.linkedItems.smartLabel, g = r.dataLabelStyle, m = h(parseInt(g.lineHeight, 10), 12), l = c.valuePadding, p = 0, e = 0, b = (b = b[e]) &&
                b.config;
                f.useEllipsesOnOverflow(r.useEllipsesWhenOverflow);
                for (f.setStyle(g); 1 > e; e += 1)
                    b && b.displayValue !== d && void 0 !== b.displayValue && c.showValue && (r = f.getOriSize(b.displayValue), b.displayValue === d && (r = {
                    height: m
                }), 0 < r.height && (p = r.width + l + 2), p > a && (p = a));
                c.widthUsed = p;
                return {
                    top: 0,
                    right: p
                }
            }, updateData: function(a, c, b) {
                var f = this.config, h = f.maxValue, d = f.prevMin, r = this.chart, g = this.groupManager || this, m = r.components.scale;
                this._setConfigure(a, c);
                this.setMaxMin();
                if (f.maxValue !== h || f.minValue !== d)
                    this.maxminFlag =
                    !0;
                b && (r._setAxisLimits(), m.draw(), g.draw())
            }, setMaxMin: function() {
                var a = this.components.data, c = this.config, b, f, h = a.length, d =- Infinity, r = Infinity;
                for (b = 0; b < h; b++)
                    a[b] && (f = a[b].config, d = Z(d, f.setValue, f.target), r = S(r, f.setValue, f.target));
                c.maxValue = d;
                c.minValue = r
            }, draw: function() {
                var c = this.chart, f = c.components, d = c.jsonData.chart, r = c.config, m = f.paper, l = f.canvas, p = l.graphics, y = l.config, l = p.canvasElement, v = r.canvasLeft, w = r.canvasRight, u = r.canvasTop, k = r.canvasBottom, B = r.canvasWidth, n = r.canvasHeight,
                F = c.graphics.datasetGroup, W, D = f.scale, G = D.getLimit().min, D = D.getLimit().max, I = c.isHorizontal, ia = c.get(e, C), K = ia.animType, ra = ia.animObj, J = ia.dummyObj, ia = ia.duration, Da, V, E = c.components.colorManager, S, oa, O, P, Y, T, qa, wa;
                Da = I ? 270 : 180;
                y.colorRangeFillMix = V = b.getFirstDefinedValue(d.colorrangefillmix, d.gaugefillmix, c.colorRangeFillMix, "{light-10},{dark-10},{light-10},{dark-10}");
                y.colorRangeFillRatio = c = b.getFirstDefinedValue(d.colorrangefillratio, d.gaugefillratio, c.colorRangeFillRatio, d.gaugefillratio, "0,10,80,10");
                y.colorRangeGetter = f = f.colorRange;
                y.colorArray = f = f && f.getColorRangeArr(G, D);
                S = a(d.colorrangebordercolor, d.gaugebordercolor, "{dark-20}");
                oa = h(d.colorrangeborderalpha, d.gaugeborderalpha, 100);
                O = h(d.showshadow, 1);
                P = h(d.showgaugeborder, d.showcolorrangeborder, 0);
                y.colorRangeBorderThickness = d = P ? h(d.colorrangeborderthickness, d.gaugeborderthickness, 2) : 0;
                y = f && f.length;
                if (l)
                    for (p = l.colorRangeElems.length; p > y; --p)
                        l.colorRangeElems[p - 1].hide(), l.colorRangeElems[p - 1].shadow({
                            opacity: 0
                        });
                else 
                    p.canvasElement = l =
                    {}, l.colorRangeElems = [];
                for (p = 0; p < y; p += 1)
                    P = f[p], Y = P.minvalue - G, T = P.maxvalue - G, Y = I ? I ? {
                        x: v + Y * B / (D - G),
                        y: u,
                        width: (T - Y) * B / (D - G),
                        height: n
                    } : void 0 : {
                        x: v,
                        y: u + (n - T * n / (D - G)),
                        width: B,
                        height: (T - Y) * n / (D - G)
                    }, P.x = Y.x, P.y = Y.y, P.width = Y.width, P.height = Y.height, T = P.code, T = g(N(a(P.bordercolor, T), S), h(P.borderalpha, oa)), qa = E.parseColorMix(P.code, V), wa = E.parseAlphaList(P.alpha, qa.length), W = h(P.borderAlpha, oa), P = wa.split(L), P = Z.apply(Math, P), P = Z(d && W || 0, P), W = {
                        x: Y.x,
                        y: Y.y,
                        width: Y.width,
                        height: Y.height,
                        r: 0,
                        "stroke-width": d,
                        stroke: T,
                        fill: H({
                            FCcolor: {
                                color: qa.toString(),
                                ratio: c,
                                alpha: wa,
                                angle: Da
                            }
                        })
                    }, l.colorRangeElems[p] ? (l.colorRangeElems[p].show(), W = {
                        "stroke-width": d,
                        stroke: T,
                        fill: H({
                            FCcolor: {
                                color: qa.toString(),
                                ratio: c,
                                alpha: wa,
                                angle: Da
                            }
                        })
                    }, l.colorRangeElems[p].animateWith(J, ra, {
                        x: Y.x,
                        y: Y.y,
                        width: Y.width,
                        height: Y.height,
                        r: 0
                    }, ia, K), l.colorRangeElems[p].attr(W)) : l.colorRangeElems[p] = m.rect(W, F), l.colorRangeElems[p].shadow({
                        apply: O,
                        opacity: P / 100
                    });
                r.gaugeStartX = v;
                r.gaugeEndX = w;
                r.gaugeStartY = u;
                r.gaugeEndY = k;
                r.gaugeCenterX =
                v + .5 * B;
                r.gaugeCenterY = u + .5 * n;
                r.gaugeRadius = .5 * B;
                this.drawPlot()
            }, drawPlot: function() {
                var f = this, h = f.JSONData, r = f.chart.jsonData.chart, g = f.config, y = f.chart.config.categories, k = h.data, t = y && y.length, B = k && k.length, n, F, y = f.visible, W = f.chart, G = W.getJobList(), I = W.config, J = I.canvasLeft, M = I.canvasRight, N = I.canvasTop, L = I.canvasBottom, Da = I.canvasHeight, E = I.canvasWidth, Z = W.components.paper, da = W.components.scale, X = W.graphics.datasetGroup, ba, U, V, aa = W.graphics, la = b.parseUnsafeString, oa = b.getValidValue, O = b.Raphael,
                P = g.showTooltip, Y = W.get(e, C), T = Y.animType, qa = Y.animObj, wa = Y.dummyObj, Y = Y.duration, ja, ka, Fa, Ia = f.components.data, ea, va, ga, Ha = da.max, Ca = da.min, Ha = da.yBasePos = da.getAxisPosition(0 >= Ha && 0 > Ca ? Ha : 0 < Ha && 0 <= Ca ? Ca : 0), Ja = Ca = 0, Na = g.showShadow, xa = g.plotBorderThickness, ca = g.plotRadius, Ga = f.graphics.container, Oa = f.graphics.trackerContainer, Ka = f.graphics.targetContainer;
                ga = f.graphics.trackerTargetContainer;
                var Ea = f.graphics.dataLabelContainer, La = f.graphics.shadowContainer, Ma = f.graphics.shadowTargetContainer;
                V = aa.trackerGroup;
                var Qa, Ra, Sa=!0, Pa=!1;
                Fa = aa.datalabelsGroup;
                var aa = W.config.dataLabelStyle, Wa = g.heightUsed, Ta = g.lowerLimit, Xa, Ya = g.showHoverEffect, cb = function(a) {
                    D.call(this, W, a)
                }, db = function(a) {
                    return function(c) {
                        0 !== this.data(u) && a.attr(this.data("setRolloverAttr"));
                        D.call(this, W, c, "DataPlotRollOver")
                    }
                }, eb = function(a) {
                    return function(c) {
                        0 !== this.data(u) && a.attr(this.data("setRolloutAttr"));
                        D.call(this, W, c, "DataPlotRollOut")
                    }
                }, $a = function() {
                    !1 !== f.visible ||!1 !== f._conatinerHidden && void 0 !== f._conatinerHidden || (Ga.hide(),
                    Oa.hide(), La.hide(), Ea && Ea.hide(), f._conatinerHidden=!0)
                };
                Ga || (Ga = f.graphics.container = Z.group("bar", X), y || Ga.hide());
                Ea || (Ea = f.graphics.dataLabelContainer = Z.group("datalabel", Fa));
                Ka || (Ka = f.graphics.targetContainer = Z.group("target", X).trackTooltip(!0), y || Ka.hide());
                Oa || (Oa = f.graphics.trackerContainer = Z.group("bar-hot", V), y || Oa.hide());
                ga || (ga = f.graphics.trackerTargetContainer = Z.group("target-hot", V), y || ga.hide());
                La || (La = f.graphics.shadowContainer = Z.group("shadow", X).toBack(), y || La.hide());
                Ma || (Ma =
                f.graphics.shadowTargetContainer = Z.group("shadow", X).toBack(), y || Ma.hide());
                S(t, B);
                for (t = 0; 1 > t; t++)
                    if (n = k[t], ga = (B = Ia[t]) && B.config, Xa = B.trackerConfig = {}, ea = ga.setValue, 0 > ea && (Pa=!0), va = ga.setLink, Qa = ga.colorArr, Ma=!1, B.graphics || (Ia[t].graphics = {}), X = ga.displayValue, oa(la(a(n.tooltext, h.plottooltext, r.plottooltext))), W.isHorizontal) {
                        ka = g.plotFillPercent / 100 * Da;
                        U = c(N + L) / 2 - ka / 2;
                        g.plotAsDot ? (ba = da.getAxisPosition(ea) - ka / 2, V = ka) : (Fa = Ta && Ta <= ea && 0 <= da.config.axisRange.min ? Ta : 0, ba = Pa ? da.getAxisPosition(ea) :
                        da.getAxisPosition(Fa), V = Pa ? da.getAxisPosition(0) - da.getAxisPosition(ea) : da.getAxisPosition(ea) - da.getAxisPosition(Fa));
                        V = O.crispBound(ba, U, V, ka, xa);
                        ba = V.x;
                        U = V.y;
                        ja = V.width;
                        ka = V.height;
                        Fa = ga.toolText === d ? ga.toolTipValue : ga.toolText;
                        Ra = ga.plotBorderDashStyle;
                        Xa.eventArgs = {
                            link: va,
                            value: ea,
                            displayValue: X,
                            toolText: Fa ? Fa: ""
                        };
                        Y || (Ja = ja);
                        F = {
                            x: ba,
                            y: U,
                            width: Ja || 1,
                            height: ka,
                            r: ca,
                            ishot: !0,
                            fill: H(Qa[0]),
                            stroke: H(Qa[1]),
                            "stroke-width": xa,
                            "stroke-dasharray": Ra,
                            "stroke-linejoin": K,
                            visibility: y
                        };
                        if (null !== ea) {
                            if (B.graphics.element ?
                            (B.graphics.element.show(), F = {
                                x: ba,
                                y: U,
                                width: ja,
                                height: ka || 1,
                                r: ca
                            }, ea = B.graphics.element, ea.animateWith(wa, qa, F, Y, T, Sa && $a), ea.attr({
                                ishot: !0,
                                fill: H(Qa[0]),
                                stroke: H(Qa[1]),
                                "stroke-width": xa,
                                "stroke-dasharray": Ra,
                                "stroke-linejoin": K,
                                visibility: y
                            }), ga.elemCreated=!1) : (ea = B.graphics.element = Z.rect(F, Ga), ea.animateWith(wa, qa, {
                                width: ja || 1
                            }, Y, T), Y && (Sa=!1), ga.elemCreated=!0), ea.shadow({
                                opacity: Na
                            }, La).data("BBox", V), va || P)
                                ka < m && (U -= (m - ka) / 2, ka = m), Xa.attr = {
                                    x: ba,
                                    y: U,
                                    width: ja,
                                    height: ka,
                                    r: ca,
                                    cursor: va ? "pointer":
                                    d,
                                    stroke: p,
                                    "stroke-width": xa,
                                    fill: p,
                                    ishot: !0,
                                    visibility: y
                                }
                        } else 
                            B.graphics.element && B.graphics.element.hide(), B.graphics.hotElement && B.graphics.hotElement.hide();
                            ga.target ? (oa(la(a(n.tooltext, h.targettooltext, r.targettooltext))), Fa = ga.toolTextTarget === d ? ga.toolTipValueTarget : ga.toolTextTarget, ka = g.targetFillPercent / 100 * Da, n = ba = da.getAxisPosition(ga.target), U = (N + L) / 2 - ka / 2, ka = U + ka, n = ["M", n, U, "L", ba, ka], F = {
                                stroke: g.targetColor,
                                "stroke-width": g.targetThickness,
                                "stroke-linecap": g.targetCapStyle,
                                ishot: !0,
                                "shape-rendering": ia[!1]
                            }, (ea = B.graphics.targetElement) ? (B.graphics.targetElement.show(), F = {
                                path: n,
                                stroke: g.targetColor,
                                "stroke-width": g.targetThickness,
                                "stroke-linecap": g.targetCapStyle,
                                ishot: !0,
                                "shape-rendering": ia[!1]
                            }, ea.animateWith(wa, qa, F, Y, T)) : (ea = B.graphics.targetElement = Z.path(n, Ka).attr(F), Ma=!0), Ma && ea.click(cb).hover(db(ea), eb(ea)), ea.shadow({
                                opacity: Na
                            }, La).data("BBox", V).data("eventArgs", void 0).data("groupId", void 0).data(u, Ya).data("setRolloverAttr", ga.tagetHoverAttr).data("setRolloutAttr",
                            ga.targetOutAttr), P ? ea.tooltip(Fa) : ea.tooltip(!1)) : B.graphics.targetElement && B.graphics.targetElement.hide();
                            Ma = parseInt(aa.lineHeight, 10);
                            U = .5 * (N + Da);
                            X !== d && void 0 !== X && g.showValue ? (F = {
                                text: X,
                                "text-anchor": v,
                                x: M + g.valuePadding + 2,
                                y: U,
                                "vertical-align": l,
                                fill: aa.color,
                                direction: ga.textDirection,
                                "text-bound": [aa.backgroundColor, aa.borderColor, aa.borderThickness, aa.borderPadding, aa.borderRadius, aa.borderDash]
                            }, B.graphics.label ? (B.graphics.label.show(), B.graphics.label.animateWith(wa, qa, {
                                x: M + g.valuePadding +
                                2,
                                y: U
                            }, Y, T), B.graphics.label.attr({
                                text: X,
                                "text-anchor": v,
                                "vertical-align": l,
                                fill: aa.color,
                                direction: ga.textDirection,
                                "text-bound": [aa.backgroundColor, aa.borderColor, aa.borderThickness, aa.borderPadding, aa.borderRadius, aa.borderDash]
                            })) : B.graphics.label = Z.text(F, Ea), X = B.graphics.label.getBBox(), 0 > X.x + I.marginLeft && (X = X.width - I.marginLeft, I.width < X && (X = I.width - I.marginLeft), F = {
                                x: X / 2
                            }, B.graphics.label.animateWith(wa, qa, F, Y, T))) : B.graphics.label && B.graphics.label.hide() && B.graphics.label.attr({
                                "text-bound": []
                            })
                    } else {
                        V =
                        g.plotFillPercent / 100 * E;
                        ba = c(J + M) / 2 - V / 2;
                        g.plotAsDot ? (U = da.getAxisPosition(ea) - V / 2, ka = V) : (Fa = Ta && Ta <= ea && 0 <= da.config.axisRange.min ? Ta : 0, Ha = da.getAxisPosition(Fa), U = Pa ? da.getAxisPosition(0) : da.getAxisPosition(ea), ka = Pa ? da.getAxisPosition(ea) - da.getAxisPosition(0) : da.getAxisPosition(Ta && Ta <= ea && 0 <= da.config.axisRange.min ? Ta : 0) - U);
                        V = O.crispBound(ba, U, V, ka, xa);
                        ba = V.x;
                        U = V.y;
                        ja = V.width;
                        ka = V.height;
                        Fa = ga.toolText === d ? ga.toolTipValue : ga.toolText;
                        Ra = ga.plotBorderDashStyle;
                        Xa.eventArgs = {
                            link: va,
                            value: ea,
                            displayValue: X,
                            toolText: Fa
                        };
                        Y || (Ha = U, Ca = ka);
                        F = {
                            x: ba,
                            y: Ha,
                            width: ja,
                            height: Ca || 1,
                            r: ca,
                            ishot: !0,
                            fill: H(Qa[0]),
                            stroke: H(Qa[1]),
                            "stroke-width": xa,
                            "stroke-dasharray": Ra,
                            "stroke-linejoin": K,
                            visibility: y
                        };
                        B._xPos = ba;
                        B._yPos = U + ka;
                        B._height = ka;
                        B._width = ja;
                        if (null !== ea) {
                            if (B.graphics.element ? (B.graphics.element.show(), F = {
                                x: ba,
                                y: U,
                                width: ja,
                                height: ka || 1,
                                r: ca
                            }, ea = B.graphics.element, ea.animateWith(wa, qa, F, Y, T, Sa && $a), ea.attr({
                                ishot: !0,
                                fill: H(Qa[0]),
                                stroke: H(Qa[1]),
                                "stroke-width": xa,
                                "stroke-dasharray": Ra,
                                "stroke-linejoin": K,
                                visibility: y
                            }),
                            ga.elemCreated=!1) : (ea = B.graphics.element = Z.rect(F, Ga), ea.animateWith(wa, qa, {
                                y: U,
                                height: ka || 1
                            }, Y, T), Y && (Sa=!1), ga.elemCreated=!0), ea.shadow({
                                opacity: Na
                            }, La).data("BBox", V), va || P)
                                ka < m && (U -= (m - ka) / 2, ka = m), Xa.attr = {
                                    x: ba,
                                    y: U,
                                    width: ja,
                                    height: ka,
                                    r: ca,
                                    cursor: va ? "pointer": d,
                                    stroke: p,
                                    "stroke-width": xa,
                                    fill: p,
                                    ishot: !0,
                                    visibility: y
                                }
                        } else 
                            B.graphics.element && B.graphics.element.hide(), B.graphics.hotElement && B.graphics.hotElement.hide();
                            ga.target ? (oa(la(a(n.tooltext, h.targettooltext, r.targettooltext))), Fa = ga.toolTextTarget ===
                            d ? ga.toolTipValueTarget : ga.toolTextTarget, ka = g.targetFillPercent / 100 * E, n = (J + M) / 2 - ka / 2, ba = n + ka, U = ka = da.getAxisPosition(ga.target), n = ["M", n, U, "L", ba, ka], F = {
                                stroke: g.targetColor,
                                "stroke-width": g.targetThickness,
                                "stroke-linecap": g.targetCapStyle,
                                ishot: !0,
                                "shape-rendering": ia[!1]
                            }, (ea = B.graphics.targetElement) ? (B.graphics.targetElement.show(), F = {
                                path: n,
                                stroke: g.targetColor,
                                "stroke-width": g.targetThickness,
                                "stroke-linecap": g.targetCapStyle,
                                ishot: !0,
                                "shape-rendering": ia[!1]
                            }, ea.animateWith(wa, qa, F, Y, T)) :
                            (ea = B.graphics.targetElement = Z.path(n, Ka).attr(F), Ma=!0), Ma && ea.click(cb).hover(db(ea), eb(ea)), ea.shadow({
                                opacity: Na
                            }, La).data("BBox", V).data("eventArgs", void 0).data("groupId", void 0).data(u, Ya).data("setRolloverAttr", ga.tagetHoverAttr).data("setRolloutAttr", ga.targetOutAttr), P ? ea.tooltip(Fa) : ea.tooltip(!1)) : B.graphics.targetElement && B.graphics.targetElement.hide();
                            Ma = parseInt(aa.lineHeight, 10);
                            U = Ma > Wa ? I.height - I.marginBottom - Wa + Ma / 2 : I.height - I.marginBottom - Ma / 2;
                            U -= I.borderWidth;
                            U -= (W._manageActionBarSpace &&
                            W._manageActionBarSpace(.225 * ga.availableHeight) || {}).bottom;
                            X !== d && void 0 !== X && g.showValue ? (F = {
                                text: X,
                                "text-anchor": ra,
                                x: E / 2 + J,
                                y: U,
                                "vertical-align": ra,
                                fill: aa.color,
                                direction: ga.textDirection,
                                "text-bound": [aa.backgroundColor, aa.borderColor, aa.borderThickness, aa.borderPadding, aa.borderRadius, aa.borderDash]
                            }, B.graphics.label ? (B.graphics.label.show(), B.graphics.label.animateWith(wa, qa, {
                                x: E / 2 + J,
                                y: U
                            }, Y, T), B.graphics.label.attr({
                                text: X,
                                "text-anchor": ra,
                                "vertical-align": ra,
                                fill: aa.color,
                                direction: ga.textDirection,
                                "text-bound": [aa.backgroundColor, aa.borderColor, aa.borderThickness, aa.borderPadding, aa.borderRadius, aa.borderDash]
                            })) : B.graphics.label = Z.text(F, Ea), X = B.graphics.label.getBBox(), 0 > X.x + I.marginLeft && (X = X.width - I.marginLeft, I.width < X && (X = I.width - I.marginLeft), F = {
                                x: X / 2
                            }, B.graphics.label.animateWith(wa, qa, F, Y, T))) : B.graphics.label && B.graphics.label.hide() && B.graphics.label.attr({
                                "text-bound": []
                            })
                    }
                G.trackerDrawID.push(w.addJob(f.drawTracker.bind(f), b.priorityList.tracker))
            }, drawTracker: function() {
                var a =
                this.chart, c = this.components, b = c.pool, f = a.config.plothovereffect, h = a.components.paper, d = this.graphics.trackerContainer, r, g, l, p, m, e, y = function(c) {
                    D.call(this, a, c)
                }, v = function(c) {
                    return function(b) {
                        var f = this.getData();
                        0 !== f.showHoverEffect&&!0 !== f.draged && (c.attr(this.getData().setRolloverAttr), D.call(this, a, b, "DataPlotRollOver"))
                    }
                }, w = function(c) {
                    return function(b) {
                        var f = this.getData();
                        0 !== f.showHoverEffect&&!0 !== f.draged && (c.attr(this.getData().setRolloutAttr), D.call(this, a, b, "DataPlotRollOut"))
                    }
                };
                r = (g = c.data[0]) && g.config;
                c = g.trackerConfig;
                l = g.graphics.element;
                m = g.graphics.hotElement;
                if (e = c.attr)(m = g.graphics.hotElement) ? m.attr(e): b && b.hotElement[0] ? (m = g.graphics.hotElement = b.hotElement[0], b.hotElement.splice(0, 1)): (m = g.graphics.hotElement = h.rect(e, d), p=!0);
                (m || l).data("eventArgs", c.eventArgs).data(u, f).data("setRolloverAttr", r.setPlotRolloverAttr || {}).data("setRolloutAttr", r.setPlotRolloutAttr || {}).tooltip(c.eventArgs && c.eventArgs.toolText);
                (p || r.elemCreated) && (m || l).click(y).hover(v(l),
                w(l))
            }, addData: function() {}, removeData: function() {}, getDataLimits: function() {
                for (var a = this.config, c = this.pointerArr && this.pointerArr.pointer, b = this.chart.jsonData.colorrange, b = b && b.color, c = c && c.length, f = a.upperLimit, h = a.lowerLimit, r, d, g = a.maxValue, l = a.minValue, c = b && b.length, a = 0; a < c; a++)
                    r = Number(b[a].maxvalue), d = Number(b[a].minvalue), f && r > f && (r = f), h && d < h && (d = h), g = Z(g, r), l = S(l, d);
                return {
                    forceMin: !0,
                    forceMax: !0,
                    max: g,
                    min: l
                }
            }
        }, "hlineargauge"])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-led",
    function() {
        var b = this.hcLib, k = b.BLANKSTRING, d = b.pluck, a = b.pluckNumber, n = b.plotEventHandler, L = b.getColorCodeString, D = b.graphics.getDarkColor, u = b.graphics.getLightColor, e = b.graphics.convertColor, C = b.preDefStr, N = C.colors.c000000, J = C.configStr, K = C.animationObjStr, I = C.showHoverEffectStr, G = C.POSITION_MIDDLE, Z = b.COMMASTRING, C = this.window, S = "rgba(192,192,192," + (/msie/i.test(C.navigator.userAgent)&&!C.opera ? .002 : 1E-6) + ")", C = Math, c = C.round, F = C.max, g = b.regex.dropHash, H = b.HASHSTRING, B = b.toRaphaelColor, f = b.HUNDREDSTRING;
        E.register("component", ["dataset", "led", {
            init: function(c) {
                var b = this.chart, f = b.components;
                if (!c)
                    return !1;
                this.JSONData = c;
                this.yAxis = f.scale;
                this.chartGraphics = b.chartGraphics;
                this.components = {};
                this.graphics = {};
                this.visible = 1 === a(this.JSONData.visible, !Number(this.JSONData.initiallyhidden), 1);
                this.configure()
            },
            draw: function() {
                var r = this.chart, y = this.config, k = r.components, p = r.jsonData.chart, C = r.config, v = r.graphics, l = k.paper, G = k.canvas, Da = G.graphics, m = G.config, h = r.get(J, K), G = h.animType, w = h.animObj,
                q = h.dummyObj, h = h.duration, z = Da.canvasBorderElement, A = Da.canvasElement, x = Da.canvasElementPath, na = Da.canvasHotElement, ta = C.canvasLeft, t = C.canvasRight, pa = C.canvasTop, ma = C.canvasBottom, R = C.canvasWidth, E = C.canvasHeight, v = v.datasetGroup, Q = k.scale, ya = Q.getLimit().min, ua = Q.getLimit().max, M = a(r.jsonData.chart.reverseaxis, r.isAxisReverse), Ba = r.isHorizontal, za = function(a, c) {
                    if (M&&!Ba)
                        return {
                            x: ta,
                            y: pa + a * E / (ua - ya),
                            width: R,
                            height: (c - a) * E / (ua - ya)
                        };
                    if (M || Ba) {
                        if (M && Ba)
                            return {
                                x: ta + (R - c * R / (ua - ya)),
                                y: pa,
                                width: (c - a) *
                                R / (ua - ya),
                                height: E
                            };
                        if (!M && Ba)
                            return {
                                x: ta + a * R / (ua - ya),
                                y: pa,
                                width: (c - a) * R / (ua - ya),
                                height: E
                            }
                    } else 
                        return {
                            x: ta,
                            y: pa + (E - c * E / (ua - ya)),
                            width: R,
                            height: (c - a) * E / (ua - ya)
                        }
                }, fa, sa, ha, da = r.components.colorManager, X, ba, U, V, aa, la, oa, O, P, Y, T, qa, wa, ja, ka, Fa, Ia, ea, va, ga, Ha, Ca, Ja;
                V = r.components.numberFormatter;
                var Na, xa, ca, Ga = y.showHoverEffect, Oa = function(a) {
                    n.call(this, r, a)
                }, Ka = function(a) {
                    var c = 0, b, f, h;
                    return function() {
                        h = this;
                        if (0 !== h.data(I))
                            for (c = 0, b = a.length; c < b; c += 1)
                                f = a[c], f.attr(h.data("setRolloverAttr")[c])
                    }
                },
                Ea = function(a) {
                    var c = 0, b, f, h;
                    return function() {
                        h = this;
                        if (0 !== h.data(I))
                            for (c = 0, b = a.length; c < b; c += 1)
                                f = a[c], f.attr(h.data("setRolloutAttr")[c])
                    }
                }, La = [], Ma = [], Qa = this.graphics.trackerContainer, Q = r.graphics.trackerGroup, Ra = 0, Sa, Pa, Wa=!1;
                Qa || (Qa = this.graphics.trackerContainer = l.group("led-hot", Q));
                ba = a(p.showgaugeborder, 1);
                Q = d(p.gaugebordercolor, r.gaugeBorderColor, "333333");
                ba = ba ? a(p.gaugeborderthickness, r.gaugeBorderThickness, 2) : 0;
                X = d(p.gaugeborderalpha, f);
                m.gaugeFillColor = Fa = d(p.gaugefillcolor, p.ledbgcolor,
                N);
                qa = a(p.usesamefillcolor, 0);
                wa = a(p.usesamefillbgcolor, qa);
                y.ledGap = ja = a(p.ledgap, 2);
                y.ledSize = ka = a(p.ledsize, 2);
                a(p.showhovereffect, 0);
                m.colorRangeFillMix = fa = b.getFirstDefinedValue(p.colorrangefillmix, p.gaugefillmix, r.colorRangeFillMix, "{light-10},{dark-10},{light-10},{dark-10}");
                m.colorRangeFillRatio = sa = b.getFirstDefinedValue(p.colorrangefillratio, p.gaugefillratio, r.colorRangeFillRatio, p.gaugefillratio, "0,10,80,10");
                m.colorRangeGetter = X = k.colorRange;
                m.colorArray = ha = X && X.getColorRangeArr(ya, ua);
                Q = d(Q, N).replace(g, H);
                X = a(p.colorrangeborderalpha, p.gaugeborderalpha, 100);
                k = a(p.showshadow, 1);
                ba = a(p.showgaugeborder, 1);
                m.colorRangeBorderThickness = ba = ba ? a(p.colorrangeborderthickness, p.gaugeborderthickness, 2) : 0;
                m = ja + ka || 1;
                Ia = (Ba ? R : E) - ka;
                U = ua - ya;
                ea = ba / 2;
                va = ta - ea;
                ga = pa - ea;
                Ha = ta + R + ea;
                ea = pa + E + ea;
                Ca = parseInt(Ia / m, 10) + 1;
                y.sizeGapSum = m = ka + Ia%m / Ca + ja;
                y.perLEDValueLength = Ia = U / Ca;
                y = ta;
                ka = pa;
                p = d(p.clickurl);
                A || (Da.canvasElement = A = {}, A.colorRangeElems = []);
                xa = V.getCleanValue(r.jsonData.value);
                if (qa || wa)
                    for (V =
                    0, aa = ha.length; V < aa; V += 1)
                        if (xa >= ha[V].minvalue && xa <= ha[V].maxvalue) {
                            Ja = ha[V].code || da.getPlotColor(V);
                            ca = V;
                            break
                        }
                ha && 0 < ha.length && (Na = ha[0].code || da.getPlotColor(0));
                Sa = M ? t : ta;
                Pa = M ? pa : ma;
                V = 0;
                for (aa = ha && ha.length; V < aa; V += 1)
                    U = ha[V], la = za(U.minvalue - ya, U.maxvalue - ya), oa = c((U.maxvalue - ya) / Ia), O = oa - Ra, Ra = oa, oa = O * m, Ba || M?!Ba && M ? (la.height = oa - ja, la.y = Pa, Pa += oa) : Ba&&!M ? (la.width = oa - ja, la.x = Sa, Sa += oa) : Ba && M && (la.width = oa - ja, la.x = Sa - la.width, Sa -= oa) : (la.height = oa - ja, la.y = Pa - la.height, Pa -= oa), U.x = la.x, U.y = la.y,
                U.width = la.width, U.height = la.height, oa = qa ? Ja : wa && V > ca ? Na : Na = U.code || da.getPlotColor(V), O = e(L(d(U.bordercolor, oa), Q), a(U.borderalpha, X)), P = da.parseColorMix(U.code, fa), Y = da.parseAlphaList(U.alpha, P.length), P = a(U.borderAlpha, X), T = Y.split(Z), T = F.apply(Math, T), T = F(ba && P || 0, T), P = {
                    x: la.x,
                    y: la.y,
                    width: 0 > la.width ? 0: la.width,
                    height: 0 > la.height ? 0: la.height,
                    r: 0,
                    "stroke-width": 0,
                    stroke: O,
                    fill: B({
                        FCcolor: {
                            color: oa,
                            ratio: sa,
                            alpha: Y,
                            angle: 180
                        }
                    })
                }, A.colorRangeElems[V] ? (A.colorRangeElems[V].show(), A.colorRangeElems[V].animateWith(q,
                w, {
                    x: la.x,
                    y: la.y,
                    width: 0 > la.width ? 0: la.width,
                    height: 0 > la.height ? 0: la.height,
                    r: 0
                }, h, G), A.colorRangeElems[V].attr({
                    "stroke-width": 0,
                    stroke: O,
                    fill: B({
                        FCcolor: {
                            color: oa,
                            ratio: sa,
                            alpha: Y,
                            angle: 180
                        }
                    })
                })) : A.colorRangeElems[V] = l.rect(P, v).toBack(), A.colorRangeElems[V].shadow({
                    apply: k,
                    opacity: T / 100
                }), La.push({
                    "stroke-width": 0,
                    fill: B({
                        FCcolor: {
                            color: D(d(oa, N), 80) + Z + u(d(oa, N), 80),
                            alpha: a(U.alpha, 100),
                            angle: Ba ? 90: 0
                        }
                    })
                }), Ma.push({
                    "stroke-width": 0,
                    fill: B({
                        FCcolor: {
                            color: d(oa, N),
                            alpha: a(U.alpha, 100)
                        }
                    })
                });
                if (ha &&
                0 === ha.length)
                    for (V = 0, aa = A.colorRangeElems.length; V < aa; V++)
                        A.colorRangeElems[V].hide();
                Ja = {
                    link: p,
                    value: xa
                };
                P = {
                    x: ta,
                    y: pa,
                    width: R,
                    height: E,
                    "stroke-width": 0,
                    fill: S,
                    ishot: !0
                };
                na ? na.attr(P) : (na = Da.canvasHotElement = l.rect(P, Qa), Wa=!0);
                na.data("eventArgs", Ja).data(I, Ga).data("setRolloverAttr", La).data("setRolloutAttr", Ma);
                Wa && (p && na.click(Oa), na.hover(Ka(A.colorRangeElems), Ea(A.colorRangeElems)));
                Ba ? y += m - ja / 2 : ka += m - ja / 2;
                Ja = [];
                P = {
                    path: ["M", va, ga, "L", Ha, ga, Ha, ea, va, ea, "Z"],
                    stroke: e(Q, X),
                    "stroke-width": ba,
                    "stroke-linecap": "butt"
                };
                z ? (z.animateWith(q, w, {
                    path: ["M", va, ga, "L", Ha, ga, Ha, ea, va, ea, "Z"]
                }, h, G), z.attr({
                    stroke: e(Q, X),
                    "stroke-width": ba
                })) : Da.canvasBorderElement = l.path(P, v).shadow({
                    apply: k
                }).toBack();
                for (V = 1; V < Ca; V += 1)
                    Ba ? (Ja.push("M", y, ka, "L", y, ka + E), y += m) : (Ja.push("M", y, ka, "L", y + R, ka), ka += m);
                P = {
                    path: Ja,
                    stroke: e(Fa, 100),
                    "stroke-width": ja,
                    "stroke-linecap": "butt"
                };
                x ? (x.animateWith(q, w, {
                    path: Ja
                }, h, G), x.attr({
                    stroke: e(Fa, 100),
                    "stroke-width": ja
                })) : Da.canvasElementPath = l.path(P, v);
                C.gaugeStartX = ta;
                C.gaugeEndX =
                t;
                C.gaugeStartY = pa;
                C.gaugeEndY = ma;
                C.gaugeCenterX = ta + .5 * R;
                C.gaugeCenterY = pa + .5 * E;
                C.gaugeRadius = .5 * R;
                this.drawShade()
            },
            drawShade: function() {
                var b = this.config, f = this.components.data, d = this.chart, g = d.config, B = g.canvasLeft, v = g.canvasTop, l = g.canvasHeight, H = g.canvasWidth, u = d.components.paper, m = d.graphics.datasetGroup, h = this.graphics.container, w, q = d.components.numberFormatter, z = a(this.chart.jsonData.chart.reverseaxis, d.isaxisreverse), A = d.isHorizontal, x = d.get(J, K), n = x.animType, F = x.animObj, t = x.dummyObj, x = x.duration,
                f = f[0], C = f.graphics, D = f && f.config, I = this.graphics.dataLabelContainer, N = d.graphics.datalabelsGroup, Q = d.config.dataLabelStyle, C = b.heightUsed, L = b.initAnimation, E;
                E = d.components.scale.getLimit().min;
                w = d.components.canvas.config.gaugeFillColor;
                h || (h = this.graphics.container = u.group("shade", m));
                I || (I = this.graphics.dataLabelContainer = u.group("datalabel", N));
                q = q.getCleanValue(D.setValue);
                f.graphics || (f.graphics = {});
                m = f.graphics.element;
                q = c((q - E) / b.perLEDValueLength) * b.sizeGapSum - b.ledGap;
                E = Math.ceil(l - q);
                N = Math.ceil(H - q);
                z&&!A ? (z = {
                    x: B,
                    y: x && L ? v: v + q,
                    width: H,
                    height: x && L ? l: E,
                    r: 0,
                    "stroke-width": 0,
                    fill: e(w, 50)
                }, m ? (m.animateWith(t, F, {
                    x: B,
                    y: v + q,
                    width: H,
                    height: E,
                    r: 0
                }, x, n), m.attr({
                    "stroke-width": 0,
                    fill: e(w, 50)
                })) : (m = f.graphics.element = u.rect(z, h), m.animateWith(t, F, {
                    y: v + q,
                    height: E
                }, x, n))) : z || A?!z && A ? (z = {
                    x: x && L ? B: B + q,
                    y: v,
                    width: x && L ? H: N,
                    height: l,
                    r: 0,
                    "stroke-width": 0,
                    fill: e(w, 50)
                }, m ? (m.animateWith(t, F, {
                    x: B + q,
                    y: v,
                    width: N,
                    height: l,
                    r: 0
                }, x, n), m.attr({
                    "stroke-width": 0,
                    fill: e(w, 50)
                })) : (m = f.graphics.element = u.rect(z,
                h), m.animateWith(t, F, {
                    x: B + q,
                    width: N
                }, x, n))) : z && A && (z = {
                    x: B,
                    y: v,
                    width: x && L ? H: N,
                    height: l,
                    r: 0,
                    "stroke-width": 0,
                    fill: e(w, 50)
                }, m ? (m.animateWith(t, F, {
                    x: B,
                    y: v,
                    width: N,
                    height: l,
                    r: 0
                }, x, n), m.attr({
                    "stroke-width": 0,
                    fill: e(w, 50)
                })) : (m = f.graphics.element = u.rect(z, h), m.animateWith(t, F, {
                    width: N
                }, x, n))) : (z = {
                    x: B,
                    y: v,
                    width: H,
                    height: x && L ? l: E,
                    r: 0,
                    "stroke-width": 0,
                    fill: e(w, 50)
                }, m ? (m.animateWith(t, F, {
                    x: B,
                    y: v,
                    width: H,
                    height: E,
                    r: 0
                }, x, n), m.attr({
                    "stroke-width": 0,
                    fill: e(w, 50)
                })) : (m = f.graphics.element = u.rect(z, h), m.animateWith(t,
                F, {
                    height: E
                }, x, n)));
                v = D.setTooltext === k || void 0 === D.setTooltext ? D.toolTipValue : D.setTooltext;
                b.showTooltip ? d.components.canvas.graphics.canvasHotElement.tooltip(v) : d.components.canvas.graphics.canvasHotElement.tooltip(!1);
                v = parseInt(Q.lineHeight, 10);
                v = v > C ? g.height - g.marginBottom - C + v / 2 : g.height - g.marginBottom - v / 2;
                v -= g.borderWidth;
                v -= (d._manageActionBarSpace && d._manageActionBarSpace(.225 * D.availableHeight) || {}).bottom;
                C = f.graphics;
                D.displayValue !== k && void 0 !== D.displayValue && b.showValue ? (z = {
                    text: D.displayValue,
                    "text-anchor": G,
                    x: H / 2 + B,
                    y: v,
                    "vertical-align": G,
                    fill: Q.color,
                    direction: D.textDirection,
                    "text-bound": [Q.backgroundColor, Q.borderColor, Q.borderThickness, Q.borderPadding, Q.borderRadius, Q.borderDash]
                }, C.label ? (C.label.animateWith(t, F, {
                    x: H / 2 + B,
                    y: v
                }, x, n), C.label.attr({
                    text: D.displayValue,
                    "text-anchor": G,
                    "vertical-align": G,
                    fill: Q.color,
                    direction: D.textDirection,
                    "text-bound": [Q.backgroundColor, Q.borderColor, Q.borderThickness, Q.borderPadding, Q.borderRadius, Q.borderDash]
                })) : C.label = u.text(z, I), b = C.label.getBBox(),
                0 > b.x + g.marginLeft && (b = b.width - g.marginLeft, g.width < b && (b = g.width - g.marginLeft), z = {
                    x: b / 2
                }, C.label.animateWith(t, F, z, x, n))) : C.label && (C.label = C.label.remove())
            }
        }, "bullet"])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-thermometer", function() {
        var b = this.hcLib, k = b.pluckNumber, d = b.pluck, a = b.COMMASTRING, n = b.BLANKSTRING, L = b.getValidValue, D = b.parseTooltext, u = b.graphics.convertColor, e = b.HUNDREDSTRING, C = b.graphics.getDarkColor, N = b.graphics.getLightColor, J = b.preDefStr, K = J.configStr, I = J.animationObjStr,
        G = J.POSITION_TOP, Z = J.POSITION_MIDDLE, S = J.gaugeFillColorStr, c = J.gaugeBorderColorStr, F = b.plotEventHandler, g = b.toRaphaelColor, b = this.window, H = "rgba(192,192,192," + (/msie/i.test(b.navigator.userAgent)&&!b.opera ? .002 : 1E-6) + ")";
        E.register("component", ["dataset", "thermometer", {
            init: function(a) {
                this.configure();
                this.setValue(a && a.data && a.data[0])
            },
            setValue: function(a, c) {
                var b = this.config, g = this.chart, e = g.jsonData.chart, g = g.components.numberFormatter, p = a.value;
                b.value = p = g.getCleanValue(p);
                null !== p ? (g = b.formattedValue =
                g.dataLabels(b.value), b.displayValue = b.showValue ? a.label || L(g, n) : n, b.toolText = b.showTooltip ? void 0 !== e.plottooltext ? D(d(a.tooltext, e.plottooltext), [1, 2], {
                    formattedValue : g
                }, {}, e) : d(a.tooltext, null === g ? n : g) : n) : (b.displayValue = n, b.toolText = n, b.formattedValue = null);
                c && this.draw()
            },
            configure: function() {
                var b = this.chart, f = b.jsonData, g = f.chart, y = this.config || (this.config = {}), H = b.components.colorManager, p = b.config, n, v = k(g.showhovereffect), l;
                y.showValue = k(g.showvalue, g.showvalues, 1);
                y.showTooltip = k(g.showtooltip,
                1);
                y.valuePadding = k(g.valuepadding, 2);
                y.tooltipSepChar = d(g.tooltipsepchar, a);
                y.pointerOnOpp = k(g.pointerontop, 0);
                y.axisPosition = k(g.ticksbelowgauge, g.ticksbelowgraph, this.ticksbelowgauge, 1) ? 3 : 1;
                y.valueAbovePointer = k(g.valueabovepointer, 1);
                y.labelStyle = p.dataLabelStyle;
                l = y.gaugeFillColor = d(g.gaugefillcolor, g.thmfillcolor, g.cylfillcolor, H.getColor(S));
                p = y.gaugeFillAlpha = k(g.gaugefillalpha, g.cylfillalpha, g.thmfillalpha, e);
                y.showGaugeBorder = k(g.showgaugeborder, 1);
                n = y.showGaugeBorder ? k(g.gaugeborderalpha,
                40) : 0;
                y.gaugeBorderColor = u(d(g.gaugebordercolor, H.getColor(c)), n);
                y.gaugeBorderThickness = k(g.gaugeborderthickness, 1);
                y.gaugeContainerColor = d(g.thmglasscolor, g.cylglasscolor, b.glasscolor, N(l, 30));
                0 !== v && (v || g.plotfillhovercolor || g.plotfillhoveralpha || 0 === g.plotfillhoveralpha) && (y.showHoverEffect=!0, v = d(g.plotfillhovercolor, g.cylfillhovercolor, g.thmfillhovercolor, "{dark-10}"), y.plotFillHoverAlpha = k(g.plotfillhoveralpha, g.cylfillhoveralpha, g.thmfillhoveralpha, p), y.plotFillHoverColor = /\{/.test(v) ? H.parseColorMix(l,
                v)[0] : v);
                this.setValue(f);
                b._parseSpecialConfig && b._parseSpecialConfig()
            },
            _getLabelSpace: function() {
                var a = this.config, c = this.chart, b = c.get("linkedItems", "smartLabel");
                b.useEllipsesOnOverflow(c.config.useEllipsesWhenOverflow);
                b.setStyle(a.labelStyle);
                c = b.getOriSize(a.displayValue !== n ? a.displayValue : "dummy");
                return c.height ? c.height + a.valuePadding : 0
            },
            _manageSpace: function() {
                var a = this.chart.config, c = a.canvasWidth, b = a.canvasHeight, g = a.canvasLeft, d = a.canvasRight, p = a.canvasTop, e = a.xDefined, v = a.yDefined,
                l = a.rDefined, H = a.hDefined, u = a.thmOriginX, m = a.thmOriginY, h = a.thmBulbRadius, w = a.thmHeight, q;
                q = a.origW;
                var z = a.origH, A = a.width, x = a.height, z = k(z, x);
                q = k(q, A);
                q = z && q ? q / A == z / x ? A / q : Math.min(A / q, x / z) : 1;
                var n = x = A = z = 0, F;
                F = this._getLabelSpace();
                b -= F;
                A += F;
                h = l ? h * q : Math.min(c / 2, .13 * k(w, b));
                a.effectiveR = h;
                l = .643 * h;
                a = 2 * l;
                z += l;
                p += l;
                b -= l;
                e ? g = u * q - l - g : (g = (d - g) / 2 - l, g + a > c && (g = c - a));
                n += c - g - a;
                v && (m*=q);
                H ? (w*=q, v ? z += m - w - p : m = p + w) : v || (w = Math.max(b - 1.766 * h, 3 * h), m = p + w);
                return {
                    top: z,
                    bottom: A + (p + b - m),
                    left: x + g,
                    right: n
                }
            },
            draw: function() {
                var c =
                this.config, b = this.chart, d = b.graphics.datalabelsGroup, e = b.get("graphics", "trackerGroup"), n = this.graphics || (this.graphics = {}), p = b.get("components", "scale"), D = b.get("graphics", "datasetGroup"), v = b.get(K), l = b.get("components", "paper"), ra = v.canvasLeft, J = v.canvasTop, m = v.canvasHeight, h = v.effectiveR || 10, w = .643 * h, q = v.use3DLighting, z = ra + w, A = J - w, x = A + w, L = x + m, ta = L + .766 * h, t = k(c.value, p.getLimit().min), pa = p.getPixel(t), p = .33 * w, t = A + p, E = .9 * w, R = w - p, S = h + E - w, Q = z - w, ya = z + w, ua = z - R, R = z + R, M = z - E, Ba = z + E, za = parseInt(z - .6 * w,
                10), w = z + w / 2, fa = ta - Math.abs(Math.sqrt(S * S - E * E)), sa = n.container, ha = n.fluid, da = n.topLightGlow, X = n.topLight, ba = n.label, U = n.dataLabelContainer, ba = n.canvasBorderElement, E = n.bulbBorderLight, V = n.bulbTopLight, aa = n.bulbCenterLight, la = n.trackerContainer, oa = n.cylLeftLight, O = n.cylRightLight, P = n.cylLeftLight1, Y = n.cylRightLight1, la = n.hotElement, T = C(c.gaugeFillColor, q ? 70 : 80), qa = c.gaugeFillAlpha, wa = c.gaugeContainerColor, ja = C(wa, 80), wa = N(wa, 80), ka = c.gaugeBorderThickness, Fa = c.gaugeBorderColor, Ia = c.showHoverEffect, ea =
                c.plotFillHoverAlpha, va = c.plotFillHoverColor, ga = b.get(K, I), Ha = ga.animType, Ca = ga.animObj, Ja = ga.dummyObj, Na = ga.duration, xa = ga && ga.duration, ca = v.canvasRight, Ga = v.canvasBottom, Oa = v.canvasWidth, Ka = b.config.dataLabelStyle, ga = function() {
                    b._animCallBack && b._animCallBack()
                }, Ea = function(a, c, b, f) {
                    "attr" === c ? a.attr(b) : a.animateWith(Ja, Ca, b, Na, Ha, f);
                    return a
                }, La, Ma = c.fluidAttr;
                Ma || (Ma = c.fluidAttr = {});
                Ia && (Ma.hover = {
                    fill: u(C(va, q ? 70 : 80), ea)
                });
                Ma.out = {
                    fill: u(T, qa)
                };
                qa = ["M", ua, A, "A", p, p, 0, 0, 0, Q, t, "L", Q, L, "A", h, h,
                0, 1, 0, ya, L, "L", ya, t, "A", p, p, 0, 0, 0, R, A, "Z"];
                v.gaugeStartX = ra;
                v.gaugeEndX = ca;
                v.gaugeStartY = J;
                v.gaugeEndY = Ga;
                v.gaugeCenterX = ra + .5 * Oa;
                v.gaugeCenterY = J + .5 * m;
                v.gaugeRadius = .5 * Oa;
                v = {
                    value: c.value,
                    displayValue: c.displayValue,
                    toolText: c.toolText
                };
                sa ? xa ? (d = e = "animate", La = ga) : d = e = "attr" : (sa = n.container = l.group("thermometer", D), ba = n.canvasBorderElement = l.path(sa), ha = n.fluid = l.path(sa).attr({
                    "stroke-width": 0
                }), X = n.topLight = l.path(sa).attr({
                    "stroke-width": 1
                }), da = n.topLightGlow = l.path(sa).attr({
                    "stroke-width": 0
                }),
                E = n.bulbBorderLight = l.path(sa).attr({
                    "stroke-width": 0,
                    stroke: "#00FF00"
                }), V = n.bulbTopLight = l.path(sa).attr({
                    "stroke-width": 0
                }), aa = n.bulbCenterLight = l.path(sa).attr({
                    "stroke-width": 0
                }), oa = n.cylLeftLight = l.path(sa).attr({
                    "stroke-width": 0
                }), O = n.cylRightLight = l.path(sa).attr({
                    "stroke-width": 0
                }), P = n.cylLeftLight1 = l.path(sa).attr({
                    "stroke-width": 0
                }), Y = n.cylRightLight1 = l.path(sa).attr({
                    "stroke-width": 0
                }), la = n.trackerContainer = l.group("col-hot", e), la = n.hotElement = l.path({
                    stroke: H,
                    fill: H,
                    ishot: !0
                }, la).click(function(a) {
                    F.call(this,
                    b, a)
                }).hover(function(a) {
                    c.showHoverEffect && n.fluid && n.fluid.attr(Ma.hover);
                    F.call(this, b, a, "DataPlotRollOver")
                }, function(a) {
                    c.showHoverEffect && n.fluid && n.fluid.attr(Ma.out);
                    F.call(this, b, a, "DataPlotRollOut")
                }), U = n.dataLabelContainer = l.group("datalabel", d), xa ? (e = "animate", La = ga, ha.attr({
                    path: ["M", M, fa, "A", S, S, 0, 1, 0, Ba, fa, "L", Ba, fa, M, fa, "Z"]
                })) : e = "attr", d = "attr");
                Ea(ha, e, {
                    path: ["M", M, fa, "A", S, S, 0, 1, 0, Ba, fa, "L", Ba, pa, M, pa, "Z"]
                }, La).attr(Ma.out);
                Ea(ba, d, {
                    path: qa
                }).attr({
                    "stroke-width": ka,
                    stroke: Fa
                });
                Ea(X, d, {
                    path: ["M", M, x, "L", Ba, x]
                }).attr({
                    stroke: u(T, 40)
                });
                Ea(la, d, {
                    path: qa
                }).tooltip(c.toolText);
                la.data("eventArgs", v);
                c.showValue ? (ba = n.label) ? Ea(ba.show(), d, {
                    x: z,
                    y: ta + h + (c.valuePadding || 0),
                    fill: Ka.color,
                    "text-bound": [Ka.backgroundColor, Ka.borderColor, Ka.borderThickness, Ka.borderPadding, Ka.borderRadius, Ka.borderDash]
                }).attr({
                    text: c.displayValue
                }) : (ba = n.label = l.text({
                    text: c.displayValue,
                    x: z,
                    y: ta + h + (c.valuePadding || 0),
                    "text-anchor": Z,
                    "vertical-align": G,
                    fill: Ka.color,
                    "text-bound": [Ka.backgroundColor,
                    Ka.borderColor, Ka.borderThickness, Ka.borderPadding, Ka.borderRadius, Ka.borderDash]
                }, U), ba.show()) : (ba = n.label) && ba.hide();
                q ? (Ea(da.show(), d, {
                    path: ["M", M, x, "L", Ba, x, Ba, t, M, t, "Z"]
                }).attr({
                    fill: g({
                        FCcolor: {
                            color: T + a + T,
                            alpha: "40,0",
                            ratio: "0,80",
                            radialGradient: !0,
                            cx: .5,
                            cy: 1,
                            r: "70%"
                        }
                    })
                }), l = ["M", Q, L, "A", h, h, 0, 0, 1, ya, L, "A", h, h, 0, 0, 0, Q, L, "A", h, h, 0, 1, 0, ya, L, "Z"], Ea(E.show(), d, {
                    path: l
                }).attr({
                    fill: g({
                        FCcolor: {
                            cx: .5,
                            cy: .5,
                            r: "50%",
                            color: ja + a + wa,
                            alpha: "0,50",
                            ratio: "78,30",
                            radialGradient: !0
                        }
                    })
                }), Ea(V.show(), d, {
                    path: l
                }).attr({
                    fill: g({
                        FCcolor: {
                            cx: .3,
                            cy: .1,
                            r: "100%",
                            color: wa + a + ja,
                            alpha: "60,0",
                            ratio: "0,30",
                            radialGradient: !0
                        }
                    })
                }), Ea(aa.show(), d, {
                    path: l
                }).attr({
                    fill: g({
                        FCcolor: {
                            cx: .25,
                            cy: .7,
                            r: "100%",
                            color: wa + a + ja,
                            alpha: "80,0",
                            ratio: "0,70",
                            radialGradient: !0
                        }
                    })
                }), Ea(oa.show(), d, {
                    path: ["M", z, A, "L", ua, A, "A", p, p, 0, 0, 0, Q, t, "L", Q, L, z, L, "Z"]
                }).attr({
                    fill: g({
                        FCcolor: {
                            color: wa + a + ja,
                            alpha: "50,0",
                            ratio: "0,80",
                            angle: 0
                        }
                    })
                }), Ea(O.show(), d, {
                    path: ["M", Q, A, "L", R, A, "A", p, p, 0, 0, 1, ya, t, "L", ya, L, Q, L, "Z"]
                }).attr({
                    fill: g({
                        FCcolor: {
                            color: wa + a + ja + a + ja,
                            alpha: "50,0,0",
                            ratio: "0,40,60",
                            angle: 180
                        }
                    })
                }), Ea(P.show(), d, {
                    path: ["M", za, t, "L", Q, t, Q, L, za, L, "Z"]
                }).attr({
                    fill: g({
                        FCcolor: {
                            color: wa + a + ja,
                            alpha: "60,0",
                            ratio: "0,100",
                            angle: 180
                        }
                    })
                }), Ea(Y.show(), d, {
                    path: ["M", za - .01, t, "L", w, t, w, L, za - .01, L, "Z"]
                }).attr({
                    fill: g({
                        FCcolor: {
                            color: wa + a + ja,
                            alpha: "60,0",
                            ratio: "0,100",
                            angle: 0
                        }
                    })
                })) : (da.hide(), E.hide(), V.hide(), aa.hide(), oa.hide(), O.hide(), P.hide(), Y.hide());
                La || ga()
            },
            getDataLimits: function() {
                var a = this.config, c, b;
                b = c = a.value;
                a.maxValue = b;
                a.minValue = c;
                return {
                    forceMin: !0,
                    forceMax: !0,
                    max: b,
                    min: c
                }
            },
            updateData: function(a, c, b) {
                c = this.config;
                var g = c.maxValue, d = c.prevMin, p = c.value, e = this.chart, v = this.groupManager || this, l = e.components.scale;
                this.setValue(a.data[0]);
                c.maxValue = p;
                c.minValue = p;
                if (c.maxValue !== g || c.minValue !== d)
                    this.maxminFlag=!0;
                b && (e._setAxisLimits(), l.draw(), v.draw())
            },
            addData: function() {},
            removeData: function() {}
        }
        ])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-cylinder", function() {
        var b = this.hcLib, k = b.preDefStr, d = b.COMMASTRING, a = b.plotEventHandler, n = k.configStr,
        L = k.animationObjStr, D = Math, u = D.max, e = D.min, C = b.graphics.convertColor, N = b.toRaphaelColor, J = b.graphics.getDarkColor, D = this.window, K = "rgba(192,192,192," + (/msie/i.test(D.navigator.userAgent)&&!D.opera ? .002 : 1E-6) + ")", I = k.POSITION_TOP, G = k.POSITION_MIDDLE, Z = b.pluckNumber, S = b.graphics.getLightColor;
        E.register("component", ["dataset", "cylinder", {
            _manageSpace: function() {
                var a = this.chart.config, b = a.canvasWidth, g = a.canvasHeight, d = a.canvasLeft, n = a.canvasTop, f = a.canvasRight, r = a.xDefined, y = a.yDefined, k = a.rDefined,
                p = a.hDefined, C = a.gaugeOriginX, v = a.gaugeOriginY, l = a.gaugeRadius, D = a.gaugeHeight, G = a.gaugeYScale, m;
                m = a.origW;
                var h = a.origH, w = a.width, q = a.height, h = Z(h, q);
                m = Z(m, w);
                m = h && m ? m / w == h / q ? w / m : Math.min(w / m, q / h) : 1;
                var z = q = w = h = 0, A;
                A = this._getLabelSpace();
                g -= A;
                w += A;
                l = k ? l * m : u(e(b, 1.2 * g) / 2, 5);
                a.effectiveR = l;
                a = 2 * l;
                G*=l;
                h += G;
                n += G;
                g -= 2 * G;
                r ? d = C * m - l - d : (d = (f - d) / 2 - l, d + a > b && (d = b - a));
                z += b - d - a;
                y && (v*=m);
                p ? (D*=m, y ? h += v - D - n : v = n + D) : y || (v = n + g);
                w = w + G + (n + g - v);
                return {
                    top: h,
                    bottom: w + 8,
                    left: q + d,
                    right: z
                }
            },
            draw: function() {
                var c = this.config,
                b = this.chart, g = this.graphics || (this.graphics = {}), e = b.graphics, k = e.datalabelsGroup, f = e.trackerGroup, r = e.datasetGroup, e = g.fluidTop, y = g.fluid, D = g.cylinterTop, p = g.frontLight1, ia = g.frontLight, v = g.front, l = g.back, ra = g.btnBorderLight, E = g.btnBorder1, m = g.btnBorder, h = g.label, w = g.dataLabelContainer, h = g.trackerContainer, h = g.hotElement, q = b.components, z = q.scale, A = b.config, q = q.paper, x = A.canvasLeft, na = A.canvasTop, ta = A.canvasHeight, t = A.effectiveR || 40, pa = Z(c.value, z.getLimit().min), ma = z.getPixel(pa), z = x + t, pa = b.config.dataLabelStyle,
                R = c.gaugeFillColor, Aa = J(R, 70), Q = S(R, 70), ya = J(Aa, 90), ua = A.gaugeFillAlpha, M = c.gaugeContainerColor, R = J(M, 80), Ba = J(M, 90), za = S(M, 80), fa = g.container, M = t * A.gaugeYScale, sa = t - 1.5, ha = na + ta, da = z - t, X = z + t, ba = da + 1.5, U = X - 1.5, V = da - 2, aa = X + 2, la = t + 2, oa = M + 2, O = ha + 4, P = O + .001, Y = .85 * t, T = z - Y, qa = z + Y, Y = Math.sqrt((1 - Y * Y / (t * t)) * M * M), wa = na + Y, ja = ha + Y, Y = na - 1, ka = A.use3DLighting, Fa = c.showHoverEffect, Ia = c.plotFillHoverAlpha, ea = c.plotFillHoverColor, va = b.get(n, L), ga = va.animType, Ha = va.animObj, Ca = va.dummyObj, Ja = va.duration, Na = va && va.duration,
                va = function() {
                    b._animCallBack && b._animCallBack()
                }, xa = function(a, c, b, f) {
                    "attr" === c ? a.attr(b) : a.animateWith(Ca, Ha, b, Ja, ga, f);
                    return a
                }, ca, Ga = c.fluidAttr, Oa = A.canvasRight, Ka = A.canvasBottom, Ea = A.canvasWidth, La = C(R, 50);
                A.gaugeStartX = x;
                A.gaugeEndX = Oa;
                A.gaugeStartY = na;
                A.gaugeEndY = Ka;
                A.gaugeCenterX = x + .5 * Ea;
                A.gaugeCenterY = na + .5 * ta;
                A.gaugeRadius = .5 * Ea;
                Ga || (Ga = c.fluidAttr = {});
                A = {
                    value: c.value,
                    displayValue: c.displayValue,
                    toolText: c.toolText
                };
                fa ? Na ? (k = f = "animate", ca = va) : k = f = "attr" : (fa = g.container = q.group("thermometer",
                r), m = g.btnBorder = q.path(fa).attr({
                    "stroke-width": 4
                }), E = g.btnBorder1 = q.path(fa).attr({
                    "stroke-width": 4
                }), ra = g.btnBorderLight = q.path(fa).attr({
                    "stroke-width": 0
                }), l = g.back = q.path(fa).attr({
                    "stroke-width": 1
                }), y = g.fluid = q.path(fa).attr({
                    "stroke-width": 0
                }), e = g.fluidTop = q.path(fa).attr({
                    "stroke-width": 2
                }), v = g.front = q.path(fa).attr({
                    "stroke-width": 1
                }), ia = g.frontLight = q.path(fa).attr({
                    "stroke-width": 0
                }), p = g.frontLight1 = q.path(fa).attr({
                    "stroke-width": 0
                }), D = g.cylinterTop = q.path(fa).attr({
                    "stroke-width": 2
                }),
                h = g.trackerContainer = q.group("col-hot", f), h = g.hotElement = q.path({
                    stroke: K,
                    fill: K,
                    ishot: !0
                }, h).click(function(c) {
                    a.call(this, b, c)
                }).hover(function(f) {
                    c.showHoverEffect && (g.fluid && g.fluid.attr(Ga.bodyHover), g.fluidTop && g.fluidTop.attr(Ga.topHover));
                    a.call(this, b, f, "DataPlotRollOver")
                }, function(f) {
                    c.showHoverEffect && (g.fluid && g.fluid.attr(Ga.bodyOut), g.fluidTop && g.fluidTop.attr(Ga.topOut));
                    a.call(this, b, f, "DataPlotRollOut")
                }), w = g.dataLabelContainer = q.group("datalabel", k), Na ? (f = "animate", ca = va, y.attr({
                    path: ["M",
                    da, ha, "A", t, u(M, 1), 0, 0, 0, X, ha, "L", X, ha, "A", t, u(M, 1), 0, 0, 1, da, ha, "Z"]
                }), e.attr({
                    path: ["M", ba, ha, "A", sa, M, 0, 0, 0, U, ha, "L", U, ha, "A", sa, M, 0, 0, 0, ba, ha, "Z"]
                })) : f = "attr", k = "attr");
                ka ? (r = za + d + R + d + za + d + R + d + Ba + d + Ba + d + R + d + za, x = N({
                    FCcolor: {
                        cx: .5,
                        cy: 0,
                        r: "100%",
                        color: Q + d + Aa,
                        alpha: ua + d + ua,
                        ratio: "0,100",
                        radialGradient: !0
                    }
                }), Ba = N({
                    FCcolor: {
                        cx: .5,
                        cy: .7,
                        r: "100%",
                        color: Q + d + Aa,
                        alpha: ua + d + ua,
                        ratio: "0,100",
                        radialGradient: !0
                    }
                }), ya = C(Q, ua), Q = za + d + R + d + za + d + za + d + R + d + za + d + R + d + za, ia.show().attr({
                    fill: N({
                        FCcolor: {
                            color: Q,
                            alpha: "40,0",
                            ratio: "0,100",
                            angle: 0
                        }
                    })
                }), xa(ia, k, {
                    path: ["M", da, ha, "A", t, M, 1, 0, 0, T, ja, "L", T, wa, "A", t, M, 0, 0, 1, da, na, "Z"]
                }), p.show().attr({
                    fill: N({
                        FCcolor: {
                            color: Q,
                            alpha: "40,0",
                            ratio: "0,100",
                            angle: 180
                        }
                    })
                }), xa(p, k, {
                    path: ["M", qa, ja, "A", t, M, 0, 0, 0, X, ha, "L", X, na, "A", t, M, 1, 0, 0, qa, wa, "Z"]
                })) : (r = za + d + R + d + R + d + R + d + R + d + R + d + R + d + za, Ba = x = C(Aa, ua), ya = C(ya), Q = R + d + R + d + R + d + R + d + R + d + R + d + R + d + R, ia.hide(), p.hide());
                Ga.bodyOut = {
                    fill: x
                };
                Ga.topOut = {
                    stroke: ya,
                    fill: Ba
                };
                Fa && (p = J(ea, 70), ia = S(ea, 70), T = J(p, 90), ka ? (Ga.bodyHover = {
                    fill: N({
                        FCcolor: {
                            cx: .5,
                            cy: 0,
                            r: "100%",
                            color: ia + d + p,
                            alpha: Ia + d + Ia,
                            ratio: "0,100",
                            radialGradient: !0
                        }
                    })
                }, Ga.topHover = {
                    stroke: C(ia, Ia),
                    fill: N({
                        FCcolor: {
                            cx: .5,
                            cy: .7,
                            r: "100%",
                            color: ia + d + p,
                            alpha: Ia + d + Ia,
                            ratio: "0,100",
                            radialGradient: !0
                        }
                    })
                }) : (Ga.bodyHover = {
                    fill: C(p, Ia)
                }, Ga.topHover = {
                    stroke: C(T),
                    fill: C(p, Ia)
                }));
                y.attr(Ga.bodyOut);
                e.attr(Ga.topOut);
                xa(y, f, {
                    path: ["M", da, ha, "A", t, u(M, 1), 0, 0, 0, X, ha, "L", X, ma, "A", t, u(M, 1), 0, 0, 1, da, ma, "Z"]
                }, ca);
                xa(e, f, {
                    path: ["M", ba, ma, "A", sa, M, 0, 0, 0, U, ma, "L", U, ma, "A", sa, M, 0, 0, 0, ba, ma, "Z"]
                });
                m.attr({
                    stroke: C(R,
                    80)
                });
                xa(m, k, {
                    path: ["M", V, O, "A", la, oa, 0, 0, 0, aa, O, "L", aa, P, "A", la, oa, 0, 0, 0, V, P, "Z"]
                });
                E.attr({
                    stroke: La
                });
                xa(E, k, {
                    path: ["M", da, O, "A", t, M, 0, 0, 0, X, O, "L", X, P, "A", t, M, 0, 0, 0, da, P, "Z"]
                });
                ra.attr({
                    fill: N({
                        FCcolor: {
                            color: za + d + R + d + za + d + za + d + R + d + Aa + d + R + d + za,
                            alpha: "50,50,50,50,50,70,50,50",
                            ratio: "0,15,0,12,0,15,43,15",
                            angle: 0
                        }
                    })
                });
                xa(ra, k, {
                    path: ["M", da, ha, "A", t, M, 0, 0, 0, X, ha, "A", t, M, 0, 0, 0, da, ha, "Z"]
                });
                l.attr({
                    stroke: La,
                    fill: N({
                        FCcolor: {
                            color: r,
                            alpha: "30,30,30,30,30,30,30,30",
                            ratio: "0,15,43,15,0,12,0,15",
                            angle: 0
                        }
                    })
                });
                xa(l, k, {
                    path: ["M", da, ha, "A", t, M, 0, 0, 0, X, ha, "L", X, na, "A", t, M, 0, 0, 0, da, na, "Z"]
                });
                v.attr({
                    stroke: La,
                    fill: N({
                        FCcolor: {
                            color: Q,
                            alpha: "30,30,30,30,30,30,30,30",
                            ratio: "0,15,0,12,0,15,43,15",
                            angle: 0
                        }
                    })
                });
                xa(v, k, {
                    path: ["M", da, ha, "A", t, M, 0, 0, 0, X, ha, "L", X, na, "A", t, M, 0, 0, 1, da, na, "Z"]
                });
                D.attr({
                    stroke: C(R, 40)
                });
                xa(D, k, {
                    path: ["M", da, Y, "A", t, M, 0, 0, 0, X, Y, "L", X, Y, "A", t, M, 0, 0, 0, da, Y, "Z"]
                });
                xa(h, k, {
                    path: ["M", da, ha, da, O + 4, "A", t, M, 0, 0, 0, X, O + 4, "L", X, ha, X, na, "A", t, M, 0, 0, 0, da, na, "Z"]
                }).tooltip(c.toolText);
                h.data("eventArgs",
                A);
                c.showValue ? (h = g.label) ? (h.show().attr({
                    text: c.displayValue
                }), xa(h, k, {
                    x: z,
                    y: ha + M + (c.valuePadding || 0) + 8,
                    fill: pa.color,
                    "text-bound": [pa.backgroundColor, pa.borderColor, pa.borderThickness, pa.borderPadding, pa.borderRadius, pa.borderDash]
                })) : (h = g.label = q.text({
                    text: c.displayValue,
                    x: z,
                    y: ha + M + (c.valuePadding || 0) + 8,
                    "text-anchor": G,
                    "vertical-align": I,
                    fill: pa.color,
                    "text-bound": [pa.backgroundColor, pa.borderColor, pa.borderThickness, pa.borderPadding, pa.borderRadius, pa.borderDash]
                }, w), h.show()) : (h = g.label) &&
                h.hide();
                ca || va()
            }
        }, "thermometer"])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-angulargauge", function() {
        var b = this, k = b.hcLib, d = k.Raphael, a = k.BLANKSTRING, n = k.pluck, L = k.toPrecision, D = k.getValidValue, u = k.pluckNumber, e = k.hasSVG, C = k.getFirstValue, N = k.graphics.convertColor, J = k.preDefStr, K = J.configStr, I = J.animationObjStr, G = k.getDashStyle, Z = k.parseTooltext, S = k.COMMASTRING, c = k.ZEROSTRING, F = k.parseUnsafeString, g = b.window, J = Math, H = J.abs, B = J.atan2, J = J.PI, f = 2 * J, r = J / 180, y = k.toRaphaelColor, W =
        void 0 !== g.document.documentElement.ontouchstart, p = k.getPosition, ia = k.plotEventHandler, v = function(a) {
            return void 0 !== a && null !== a
        }, l = k.setLineHeight, ra = k.HUNDREDSTRING, Da = function() {
            return function(a, c, b) {
                var f, g = this, l = this._Attr, p = d.vml?-1.5 : 0, e = d.vml?-1.5 : 0, y;
                l || (l = g._Attr = {});
                "string" === typeof a && v(c) && (f = a, a = {}, a[f] = c);
                if ("string" === typeof a || void 0 === a)
                    g = "angle" == a ? g._Attr[a] : g._attr(a);
                else 
                    for (f in a)
                        c = a[f], "angle" === f ? (l[f] = c, y = c * r, l.tooltipPos[0] = l.cx + l.toolTipRadius * Math.cos(y), l.tooltipPos[1] =
                        l.cy + l.toolTipRadius * Math.sin(y), l.prevValue = c, b && b.duration ? g.animate({
                            transform: "R" + c + S + p + S + e
                        }, b.duration, "easeIn") : g.attr({
                            transform: "R" + c + S + p + S + e
                        })) : g._attr(f, c);
                return g
            }
        };
        E.register("component", ["dataset", "angulargauge", {
            type: "angulargauge",
            pIndex: 2,
            customConfigFn: "_createDatasets",
            init: function() {
                this.components = this.components || {};
                this.idMap = {};
                this.configure()
            },
            configure: function() {
                var b = this.chart, f = b.config, g = b.jsonData, d = g.chart, p = g.pointers || g.dials, y = p.pointer || p.dial, x = this.components.data ||
                (this.components.data = []), g = b.components, H = g.scale, g = g.colorManager, F = this.config || (this.config = {}), t = u(d.gaugescaleangle, 180), B = u(d.gaugestartangle), I = u(d.gaugeendangle), W = v(B), K = e ? .001: .01, ia = v(I), J, L, M, E;
                M = f.displayValueCount = 0;
                for (E = y.length; M < E; M += 1)
                    x[M] = x[M] || (x[M] = {}), x[M].basewidth = y[M].basewidth, x[M].color = y[M].color, x[M].alpha = y[M].alpha, x[M].bgcolor = y[M].bgcolor, x[M].borderalpha = y[M].borderalpha, x[M].bordercolor = y[M].bordercolor, x[M].borderthickness = y[M].borderthickness, x[M].editmode = y[M].editmode,
                    x[M].id = n(y[M].id, "dial" + M), x[M].link = y[M].link, x[M].radius = y[M].radius, x[M].rearextension = y[M].rearextension, x[M].showvalue = y[M].showvalue, x[M].tooltext = y[M].tooltext, x[M].topwidth = y[M].topwidth, x[M].value = y[M].value, x[M].valuex = y[M].valuex, x[M].valuey = y[M].valuey, x[M].baseradius = y[M].baseradius, x[M].displayvalue = y[M].displayvalue, J = F.showValue = u(d.showvalue, d.showrealtimevalue, 0), J = u(x[M].showvalue, J), L = u(D(x[M].valuey)), J&&!v(L) && (f.displayValueCount += 1);
                if (360 < t||-360 > t)
                    t = 0 < t ? 360 : - 360;
                if (360 <
                I||-360 > I)
                    I%=360;
                if (360 < B||-360 > B)
                    B%=360;
                if (W && ia) {
                    if (t = B - I, 360 < t||-360 > t)
                        t%=360, I = B - t
                } else if (W) {
                    if (I = B - t, 360 < I||-360 > I)
                        I%=360, B += 0 < I?-360 : 360
                } else if (ia) {
                    if (B = I + t, 360 < B||-360 > B)
                        B%=360, I += 0 < B?-360 : 360
                } else 
                    360 === t ? (B = 180, I =- 180) : - 360 === t ? I = B =- 180 : (B = 90 + t / 2, I = B - t);
                360 === Math.abs(t) && (t += 0 < t?-K : K, I = B - t);
                I = 360 - I;
                B = 360 - B;
                if (360 < B || 360 < I)
                    B -= 360, I -= 360;
                F.gaugeStartAngle = B*=r;
                F.gaugeEndAngle = I * r;
                F.gaugeScaleAngle = t =- t * r;
                F.upperLimit = u(d.upperlimit);
                F.lowerLimit = u(d.lowerlimit);
                H.setAxisConfig({
                    startAngle: B,
                    totalAngle: - t
                });
                y = u(d.scaleonresize, 1);
                F.origW = u(d.origw, y ? b.origRenderWidth : f.width);
                F.origH = u(d.origh, y ? b.origRenderHeight : f.height);
                f.showtooltip = u(d.showtooltip, 1);
                f.autoScale = u(d.autoscale, 1);
                F.rearExtension = u(p.rearextension, 0);
                F.gaugeinnerradius = d.gaugeinnerradius;
                F.valueBelowPivot = u(d.valuebelowpivot, 0);
                F.showShadow = u(d.showshadow, 1);
                p = u(d.showgaugeborder, 1);
                F.gaugeFillMix = k.getFirstDefinedValue(d.colorrangefillmix, d.gaugefillmix, b.colorRangeFillMix, "{light-10},{light-70},{dark-10}");
                F.gaugeFillRatio =
                k.getFirstDefinedValue(d.colorrangefillratio, d.gaugefillratio, b.colorRangeFillRatio, d.gaugefillratio);
                void 0 === F.gaugeFillRatio ? F.gaugeFillRatio = ",6" : F.gaugeFillRatio !== a && (F.gaugeFillRatio = S + F.gaugeFillRatio);
                F.gaugeBorderColor = n(d.gaugebordercolor, "{dark-20}");
                F.gaugeBorderThickness = p ? u(d.gaugeborderthickness, 1) : 0;
                F.gaugeBorderAlpha = u(d.gaugeborderalpha, 100);
                b = g.parseColorMix(n(d.pivotfillcolor, d.pivotcolor, d.pivotbgcolor, g.getColor("pivotColor")), n(d.pivotfillmix, "{light-10},{light-30},{dark-20}"));
                F.pivotFillAlpha = g.parseAlphaList(n(d.pivotfillalpha, ra), b.length);
                F.pivotFillRatio = g.parseRatioList(n(d.pivotfillratio, c), b.length);
                F.pivotFillColor = b.join();
                F.pivotFillAngle = u(d.pivotfillangle, 0);
                F.isRadialGradient = "radial" == n(d.pivotfilltype, "radial").toLowerCase();
                F.showPivotBorder = u(d.showpivotborder, 0);
                F.pivotBorderThickness = u(d.pivotborderthickness, 1);
                F.pivotBorderColor = N(n(d.pivotbordercolor, g.getColor("pivotBorderColor")), 1 == F.showPivotBorder ? n(d.pivotborderalpha, ra) : c);
                f.dataLabels = f.dataLabels ||
                {};
                b = (b = C(d.valuebordercolor, a)) ? N(b, u(d.valueborderalpha, d.valuealpha, 100)) : a;
                f = f.dataLabels.style = {
                    fontFamily: n(d.valuefont, f.style.inCanfontFamily),
                    fontSize: n(d.valuefontsize, parseInt(f.style.inCanfontSize, 10)) + "px",
                    color: N(n(d.valuefontcolor, f.style.inCancolor), u(d.valuefontalpha, d.valuealpha, 100)),
                    fontWeight: u(d.valuefontbold) ? "bold": "normal",
                    fontStyle: u(d.valuefontitalic) ? "italic": "normal",
                    border: b || d.valuebgcolor ? u(d.valueborderthickness, 1) + "px solid": void 0,
                    borderColor: b,
                    borderThickness: u(d.valueborderthickness,
                    1),
                    borderPadding: u(d.valueborderpadding,
                    2),
                    borderRadius: u(d.valueborderradius,
                    0),
                    backgroundColor: d.valuebgcolor ? N(d.valuebgcolor,
                    u(d.valuebgalpha,
                    d.valuealpha,
                    100)): a,
                    borderDash: u(d.valueborderdashed,
                    0) ? G(u(d.valueborderdashlen,
                    4),
                    u(d.valueborderdashgap,
                    2),
                    u(d.valueborderthickness,
                    1)): a
                };
                l(f)
            },
            draw: function(c) {
                var d = this, l = d.chart, e = d.idMap, k = l.config, A = l.jsonData.chart, x = d.components.data, C = d.config, G = d.graphics, t = l.components, J = t.numberFormatter, ra = t.paper, E = t.colorManager, S = t.scale, Q = Number(C.gaugeOriginX),
                ya = Number(C.gaugeOriginY), ua = C.gaugeStartAngle, M = C.gaugeEndAngle, Ba = C.showShadow, za = k.showtooltip, fa = S.config.axisRange, sa = fa.min, ha = fa.max, da = G.pointGroup, X = G.dataLabelGroup, ba = ha - sa, U = M - ua, V = ba / U, aa, la = u(A.showhovereffect), oa = d.showValue = u(A.showvalue, A.showrealtimevalue, 0), O = 0, P = k.dataLabels.style, Y = {
                    fontFamily: P.fontFamily,
                    fontSize: P.fontSize,
                    lineHeight: P.lineHeight,
                    fontWeight: P.fontWeight,
                    fontStyle: P.fontStyle
                }, T = k.scaleFactor, qa = u(parseInt(P.lineHeight, 10), 12), wa = C.valueBelowPivot, ja = l.get(K,
                I), ka = ja.duration, Fa = ja.dummyObj, Ia = ja.animObj, ea = ja.animType, va = 0, ga = 0, Ha, Ca, Ja, Na = {}, xa = S.config.prevAxisMinMax || (S.config.prevAxisMinMax = {}), ca, Ga, Oa, Ka, Ea, La, Ma, Qa, Ra = x && x.length, Sa, Pa, Wa, Ta = {
                    pageX: 0,
                    pageY: 0
                }, Xa = Da(ua, M), Ya, cb = function(a, c) {
                    var b = x[this.pos], f = c[0], d = c[1], g = b.config || {};
                    if (b.editMode) {
                        Q = Number(C.gaugeOriginX);
                        ya = Number(C.gaugeOriginY);
                        Ya = p(l.linkedItems.container);
                        var h = [Q, ya], f = B(d - h[1] - Ya.top, f - h[0] - Ya.left);
                        k.rotationStartAngle = f;
                        g.dragStartY = b.value;
                        Wa = l._getDataJSON()
                    }
                },
                db = function() {
                    var a = x[this.pos], c = l.chartInstance, f;
                    if (a.editMode) {
                        (f = c && c.jsVars) && (f._rtLastUpdatedData = l._getDataJSON());
                        b.raiseEvent("RealTimeUpdateComplete", {
                            data: "&value=" + a.updatedValStr,
                            updateObject: {
                                values: [a.updatedValStr]
                            },
                            prevData: Wa.values,
                            source: "editMode",
                            url: null
                        }, c);
                        try {
                            g.FC_ChartUpdated && g.FC_ChartUpdated(c.id)
                        } catch (d) {
                            setTimeout(function() {
                                throw d;
                            }, 1)
                        }
                    }
                }, eb = function(c, b) {
                    var g = x[this.pos], l = W && (W && c.sourceEvent && c.sourceEvent.touches && c.sourceEvent.touches[0] || c) || Ta, p, r, m;
                    p =
                    b[2];
                    r = b[3];
                    var e;
                    if (g.editMode) {
                        k.fromDrag=!0;
                        Q = Number(C.gaugeOriginX);
                        ya = Number(C.gaugeOriginY);
                        sa = fa.min;
                        ha = fa.max;
                        ua = C.gaugeStartAngle;
                        M = C.gaugeEndAngle;
                        ba = ha - sa;
                        U = M - ua;
                        V = ba / U;
                        m = [Q, ya];
                        p = B(r - m[1] - Ya.top, p - m[0] - Ya.left);
                        p = k.rotationStartAngle - p;
                        r = 0 > p ? f + p : p - f;
                        p = g.config.dragStartY - p * V;
                        r = g.config.dragStartY - r * V;
                        (p < sa || p > ha) && r >= sa && r <= ha && (p = r);
                        p < sa ? p = H(r - ha) < H(p - sa) ? ha : sa : p > ha && (p = H(p - ha) < H(r - sa) ? ha : sa);
                        g.config.updatedValStr = p + a;
                        g.config.updatedVal = p;
                        r = [];
                        m = 0;
                        for (e = g.config.index; m < e; m += 1)
                            r.push(a);
                        r.push({
                            value: p,
                            animation: {
                                duration: 0,
                                transposeAnimDuration: 0,
                                initAnimDuration: 0
                            }
                        });
                        g.config.y !== p && d.updateData({
                            data: r
                        }, !0) && (g.updatedValStr = r.join("|"), g.dragStartX = Q || c.pageX || l.pageX)
                    }
                }, $a, Hb, Za, kb, lb, rb, sb, tb, ub, mb, vb, Ib, fb, Ua, nb, Va, wb, xb, ob, yb, zb, Ab, Bb, Cb, Db, ab, gb, pb, Jb, Eb, Kb, bb, Lb, Ob, hb, Mb, ib, Nb, Fb, jb, Gb, qb;
                hb = C.pivotRadius;
                void 0 === C.dataById && (C.dataById = {});
                c = c || ja;
                X || (X = G.dataLabelGroup = ra.group("datalabels").insertAfter(G.pointGroup));
                G.pointersPath || (G.pointersPath = []);
                G.pointersTpath ||
                (G.pointersTpath = []);
                G.dataLabel || (G.dataLabel = []);
                vb = function(a) {
                    k.fromDrag ? k.fromDrag=!1 : ia.call(this, l, a)
                };
                Ra || (Ra = 0);
                0 !== la && (la || A.dialborderhovercolor || A.dialborderhoveralpha || 0 === A.dialborderhoveralpha || A.dialborderhoverthickness || 0 === A.dialborderhoverthickness || A.dialbghovercolor || A.plotfillhovercolor || A.dialbghoveralpha || A.plotfillhoveralpha || 0 === A.dialbghoveralpha) && (la = 1);
                Na = S.config.axisRange;
                if (xa.min !== Na.min || xa.max !== Na.max)
                    l._drawCanvas(), l._drawAxis();
                xa && (xa.min = Na.min, xa.max = Na.max);
                for (; O < Ra; O += 1) {
                    ca = x[O];
                    ca.config = ca.config || {};
                    fb = ca.config;
                    ca.config.index = O;
                    e[ca.id] = {
                        index: O,
                        config: ca.config
                    };
                    qb=!1;
                    void 0 !== ca.id && (C.dataById[ca.id] = {
                        index: O,
                        point: ca
                    });
                    Ua = J.getCleanValue(ca.value);
                    Ua = Number(L(Ua, 10));
                    La = u(ca.rearextension, 0) * T;
                    nb = J.dataLabels(Ua);
                    Va = D(ca.displayvalue, nb, a);
                    wb = u(ca.showvalue, oa);
                    xb = u(D(ca.valuey) * T);
                    Mb = u(D(ca.valuex) * T);
                    n(ca.tooltext, ca.hovertext);
                    fb.itemValue = Ua;
                    fb.formatedVal = nb;
                    wb && v(xb);
                    ob = (ob = D(F(n(ca.tooltext, ca.hovertext)))) ? Z(ob, [1, 2], {
                        formattedValue: nb
                    },
                    ca, A) : Va;
                    yb = n(ca.color, ca.bgcolor, E.getColor("dialColor"));
                    zb = u(ca.alpha, ca.bgalpha, 100);
                    Ab = y({
                        FCcolor: {
                            color: yb,
                            alpha: zb,
                            angle: 90
                        }
                    });
                    Bb = n(ca.bordercolor, E.getColor("dialBorderColor"));
                    Cb = u(ca.borderalpha, 100);
                    Db = N(Bb, Cb);
                    ab = u(ca.borderthickness, 1);
                    Ga = u(ca.radius) * T;
                    Oa = u(u(ca.basewidth * T, 1.6 * hb));
                    Ka = u(ca.topwidth, 0) * T;
                    Ea = u(ca.baseradius, 0);
                    aa = u(ca.editmode, A.editmode, 0);
                    $a = n(ca.link, a);
                    ib = ob;
                    fb.toolText = ib;
                    Nb = n(Va, a);
                    Va = wb ? n(Va, a) : a;
                    gb = u(ca.showhovereffect, la);
                    if (0 !== gb && (gb || ca.borderhovercolor ||
                    ca.borderhoveralpha || 0 === ca.borderhoveralpha || ca.borderhoverthickness || 0 === ca.borderhoverthickness || ca.bghovercolor || ca.bghoveralpha || 0 === ca.bghoveralpha)) {
                        gb=!0;
                        Ja = {};
                        Ca = {};
                        pb = n(ca.borderhovercolor, A.dialborderhovercolor, "{dark-10}");
                        Jb = u(ca.borderhoveralpha, A.dialborderhoveralpha, Cb);
                        if (Eb = u(ca.borderhoverthickness, A.dialborderhoverthickness, ab))
                            Ja.stroke = Db, Kb = /\{/.test(pb), Ca.stroke = N(Kb ? E.parseColorMix(Bb, pb)[0] : pb, Jb);
                        Eb !== ab && (Ca["stroke-width"] = Eb, Ja["stroke-width"] = ab);
                        bb = n(ca.bghovercolor,
                        A.dialbghovercolor, A.plotfillhovercolor, "{dark-10}");
                        Lb = u(ca.bghoveralpha, A.dialbghoveralpha, A.plotfillhoveralpha, zb);
                        Ja.fill = Ab;
                        bb = (Ob = /\{/.test(bb)) ? E.parseColorMix(yb, bb).join() : bb;
                        Ha = {
                            FCcolor: {
                                color: bb,
                                alpha: Lb,
                                angle: 90
                            }
                        };
                        Ca.fill = y(Ha)
                    }
                    Za = ca.rolloverProperties = {
                        enabled: gb,
                        hasHoverSizeChange: void 0,
                        hoverRadius: u(lb * T),
                        baseHoverWidth: u(rb * T,
                        1.6 * hb),
                        topHoverWidth: u(tb * T),
                        rearHoverExtension: u(mb * T),
                        hoverFill: Ha,
                        hoverAttr: Ca,
                        outAttr: Ja
                    };
                    Ga = u(Ga, (Number(C.gaugeOuterRadius) + Number(C.gaugeInnerRadius)) /
                    2);
                    Ma = Oa / 2;
                    Qa = Ka / 2;
                    ca.tooltipPos = [Q, ya];
                    $a = ca.editMode ? void 0 : ca.link;
                    kb = ["M", Ga, - Qa, "L", Ga, Qa, - La, Ma, - La, - Ma, "Z"];
                    Za.hasHoverSizeChange && (Za.outAttr.path = kb, lb = u(Za.hoverRadius, Ga), rb = Za.baseHoverWidth, sb = rb / 2, tb = Za.topHoverWidth, ub = tb / 2, mb = Za.rearHoverExtension, Za.hoverAttr.path = ["M", lb, - ub, "L", lb, ub, - mb, sb, - mb, - sb, "Z"]);
                    Hb = {
                        link: ca.link,
                        value: Ua,
                        displayValue: Nb,
                        toolText: ib
                    };
                    G.pointersTpath[O] ? Ka ? (G.pointersPath[O].animateWith(Fa, Ia, {
                        path: kb
                    }, ka, ea), G.pointersTpath[O]._attr({
                        trianglepath: [0, 0, 0,
                        0, 0, 0, 0, 0, 0]
                    })) : (G.pointersTpath[O].animateWith(Fa, Ia, {
                        trianglepath: [Ga, Qa, - La, Ma, - La, - Ma, 0, Ea, Ea]
                    }, ka, ea), G.pointersPath[O]._attr({
                        path: "M00"
                    })) : (Ka ? (G.pointersPath[O] = ra.path(kb, da), G.pointersTpath[O] = ra.trianglepath(0, 0, 0, 0, 0, 0, 0, 0, 0, da)) : (G.pointersPath[O] = ra.path(["M", 0, 0], da), G.pointersTpath[O] = ra.trianglepath(Ga, Qa, - La, Ma, - La, - Ma, 0, Ea, Ea, da)), qb=!0);
                    Pa = Ka ? G.pointersPath[O] : G.pointersTpath[O];
                    G.pointersPath[O]._attr || (G.pointersPath[O]._attr = G.pointersPath[O].attr, G.pointersPath[O].attr = Xa);
                    G.pointersTpath[O]._attr || (G.pointersTpath[O]._attr = G.pointersTpath[O].attr, G.pointersTpath[O].attr = Xa);
                    Pa._attr({
                        fill: Ab,
                        stroke: Db,
                        ishot: !0,
                        "stroke-width": ab
                    }).data("eventArgs", Hb);
                    (Oa || Ka || ab) && Pa.shadow({
                        apply: Ba
                    });
                    Pa._Attr = {
                        tooltipPos: ca.tooltipPos,
                        cx: Q,
                        cy: ya,
                        toolTipRadius: Ga - La,
                        color: ca.color
                    };
                    qb && (Sa = ua / r, Pa.attr({
                        angle: Sa
                    }));
                    ca.index = O;
                    ca.editMode = aa;
                    Pa.css({
                        cursor: aa || $a ? "pointer": "default",
                        _cursor: aa ? "hand": "default"
                    }).attr({
                        ishot: !0
                    });
                    qb && (G.pointersPath[O].drag(eb, cb, db, {
                        pos: O
                    }, {
                        pos: O
                    },
                    {
                        pos: O
                    }).click(vb), G.pointersTpath[O].drag(eb, cb, db, {
                        pos: O
                    }, {
                        pos: O
                    }, {
                        pos: O
                    }).click(vb));
                    fb.y = Ua;
                    Ua >= sa && Ua <= ha && (Ib = (Ua - sa) / ba * U, Sa = (ua + Ib) / r, Pa.attr({
                        angle: Sa
                    }, null, c), za && ib !== a ? (Pa.tooltip(ib), Pa.trackTooltip(!0)) : Pa.trackTooltip(!1));
                    ga += 1;
                    v(Va) && Va !== a && (Fb = ya + (wa ? qa / 2 + hb + 2 : - (qa / 2) - hb - 2), jb = xb, Gb = u(Mb, Q), v(jb) || (jb = wa ? Fb + qa * va : Fb - qa * va), G.dataLabel[O] ? (G.dataLabel[O].attr({
                        text: Va,
                        title: ca.originalText || a,
                        fill: P.color,
                        "text-bound": [P.backgroundColor, P.borderColor, P.borderThickness, P.borderPadding,
                        P.borderRadius, P.borderDash]
                    }).css(Y).tooltip(ca.originalText), G.dataLabel[O].animateWith(Fa, Ia, {
                        x: Gb,
                        y: jb
                    }, ka, ea)) : G.dataLabel[O] = ra.text(X).attr({
                        x: Gb,
                        y: jb,
                        text: Va,
                        direction: k.textDirection,
                        fill: P.color,
                        "text-bound": [P.backgroundColor, P.borderColor, P.borderThickness, P.borderPadding, P.borderRadius, P.borderDash]
                    }).css(Y).tooltip(ca.originalText), va += 1)
                }
                O = ga;
                for (Ra = G.pointersPath.length; O < Ra; O += 1)
                    G.pointersPath[O].attr({
                        path: ["M", 0, 0]
                    }), G.pointersTpath[O].attr({
                        trianglepath: [0, 0, 0, 0, 0, 0, 0, 0, 0]
                    });
                O =
                va;
                for (Ra = G.dataLabel.length; O < Ra; O += 1)
                    G.dataLabel[O].attr({
                        text: a,
                        "text-bound": []
                    })
            },
            updateData: function(a, c) {
                if (a === this.lastUpdatedObj)
                    return !1;
                var b = this.chart, f = this.components.data, d, g, l = (g = this.components.data) && g.length || 0, p, r;
                a = a.data;
                if (l) {
                    for (; l--;)
                        if (d = a[l])
                            r = d.animation || b.get(K, I), p = d.value, g = d.tooltext, d = d.label, f[l].value = u(p, f[l].value, 0), f[l].tooltext = n(g, f[l].value), f[l].displayvalue = d;
                    this.lastUpdatedObj = a;
                    c && this.draw(r);
                    return !0
                }
            }
        }, "hlineargauge"])
    }
    ]);
    E.register("module", ["private",
    "modules.renderer.js-dataset-bulb", function() {
        var b = this.hcLib, k = b.pluckNumber, d = b.pluck, a = b.COMMASTRING, n = b.COMMASPACE, L = b.BLANKSTRING, D = b.extend2, u = b.toRaphaelColor, e = b.schedular, C = b.getValidValue, N = b.graphics.convertColor, J = b.graphics.getDarkColor, K = b.graphics.getLightColor, I = b.preDefStr, G = b.parseConfiguration, Z = b.parseUnsafeString, S = I.configStr, c = I.animationObjStr, F = I.POSITION_TOP, g = I.POSITION_MIDDLE, H = I.showHoverEffectStr, B = I.visibleStr, f = I.ROUND, r = b.plotEventHandler, y = Math.min, I = this.window,
        W = "rgba(192,192,192," + (/msie/i.test(I.navigator.userAgent)&&!I.opera ? .002 : 1E-6) + ")";
        E.register("component", ["dataset", "bulb", {
            pIndex: 2,
            customConfigFn: "_createDatasets",
            _manageSpace: function(a) {
                var c = this.config, b = this.components.data, f = this.chart, d = f.config, g = f.linkedItems.smartLabel, r = d.dataLabelStyle, f = k(parseInt(r.lineHeight, 10), 12), h = d.valuepadding, e = 0, b = (b = b[0]) && b.config;
                g.useEllipsesOnOverflow(d.useEllipsesWhenOverflow);
                g.setStyle(r);
                b && b.displayValue !== L&&!d.placevaluesinside && c.showValue &&
                (d = g.getOriSize(b.displayValue), b.displayValue === L && (d = {
                    height: f
                }), 0 < d.height && (e = d.height + h), e > a && (e = a));
                c.heightUsed = e;
                return {
                    top: 0,
                    bottom: e
                }
            },
            configure: function() {
                var a = D({}, this.JSONData), c = this.chart, b = this.config, f = c.config, g = c.jsonData.chart;
                k(g.is3d, 1);
                k(g.showtooltip, 1);
                this.__setDefaultConfig();
                G(a, b, c.config, {
                    data: !0
                });
                k(g.is3d, 1);
                b.origW = k(g.origw, f.autoscale ? c.origRenderWidth : f.width || c.origRenderWidth);
                b.origH = k(g.origh, f.autoscale ? c.origRenderHeight : f.height || c.origRenderHeight);
                k(g.showtooltip,
                1);
                b.setToolText = C(Z(d(g.plottooltext, void 0)));
                b.useColorNameAsValue = k(g.usecolornameasvalue, 0);
                b.enableAnimation = a = k(g.animation, g.defaultanimation, 1);
                b.animation = a ? {
                    duration: 1E3 * k(g.animationduration, 1)
                } : !1;
                b.showValue = k(g.showvalue, 1);
                this._setConfigure()
            },
            _setConfigure: function(c) {
                var f = this.chart, g = this.config, l = f.config, r = this.JSONData, e = c || r.data, m = e && e.length, m = c && c.data.length || m, h = f.jsonData.chart, y = f.components.colorManager, q = k(h.showtooltip, 1), H = b.parseUnsafeString;
                H(d(h.tooltipsepchar,
                n));
                var F = b.parseTooltext, x, B, G = l.showhovereffect, t = this.components.data, D, I, W, L = k(h.is3d, 1), E, Z = f.components.numberFormatter, S, M, Ba, za, fa, sa, ha, da, X, ba, U, V, aa, la, oa, O = function(c, b, f) {
                    return f ? {
                        FCcolor: {
                            cx: .4,
                            cy: .4,
                            r: "80%",
                            color: K(c, 65) + a + K(c, 75) + a + J(c, 65),
                            alpha: b + a + b + a + b,
                            ratio: "0,30,70",
                            radialGradient: !0
                        }
                    } : N(c, b)
                }, P;
                t || (t = this.components.data = []);
                for (E = 0; E < m; E++)
                    D = c ? c && c.data[E] : e[E], I = (x = t[E]) && x.config, x || (x = t[E] = {}), x.config || (I = t[E].config = {}), I.setValue = x = Z.getCleanValue(D.value) || 0, x = Z.dataLabels(x),
                    W = C(H(D.displayvalue)), I.colorRangeGetter = S = f.components.colorRange, M = S.getColorObj(I.setValue), M = M.colorObj || M.prevObj || M.nextObj, Ba = k(h.gaugefillalpha, M.alpha, 100), S = H(d(M.label, M.name)), B = d(M.bordercolor, h.gaugebordercolor, J(M.code, 70)), za = k(M.borderalpha, h.gaugeborderalpha, "90") * Ba / 100, B = (fa = /\{/.test(B)) ? y.parseColorMix(d(M.bordercolor, M.code), B)[0] : B, I.gaugeBorderColor = ha = N(B, za), I.gaugeBorderThickness = da = l.showgaugeborder ? k(h.gaugeborderthickness, 1) : 0, I.fillColor = sa = O(M.code, Ba, L), 0 !== G && (G ||
                    h.gaugefillhovercolor || h.plotfillhovercolor || h.gaugefillhoveralpha || h.plotfillhoveralpha || 0 === h.gaugefillhoveralpha || h.is3donhover || 0 === h.is3donhover || h.showgaugeborderonhover || 0 === h.showgaugeborderonhover || h.gaugeborderhovercolor || h.gaugeborderhoveralpha || 0 === h.gaugeborderhoveralpha || h.gaugeborderhoverthickness || 0 === h.gaugeborderhoverthickness) && (G=!0, X = d(h.gaugefillhovercolor, h.plotfillhovercolor, "{dark-10}"), ba = k(h.gaugefillhoveralpha, h.plotfillhoveralpha), U = k(h.showgaugeborderonhover), void 0 ===
                    U && (U = h.gaugeborderhovercolor || h.gaugeborderhoveralpha || 0 === h.gaugeborderhoveralpha || h.gaugeborderhoverthickness || 0 === h.gaugeborderhoverthickness ? 1 : l.showgaugeborder), V = d(h.gaugeborderhovercolor, "{dark-10}"), aa = k(h.gaugeborderhoveralpha), la = U ? k(h.gaugeborderhoverthickness, da || 1) : 0, oa=!!k(h.is3donhover, L), k(h.showhoveranimation, 1), U = {}, P = {}, da !== la && (U["stroke-width"] = la, P["stroke-width"] = da), P.fill = u(sa), X = (sa = /\{/.test(X)) ? y.parseColorMix(M.code, X)[0] : d(X, M.code), U.fill = u(O(X, k(ba, Ba), oa)), la && (P.stroke =
                    ha, M = /\{/.test(V), U.stroke = N(M ? y.parseColorMix(fa ? X : B, V)[0] : V, k(aa, za)))), I.setTooltext = b.getValidValue(H(d(D.tooltext, r.plottooltext, h.plottooltext))), B = q ? void 0 !== I.setTooltext ? F(I.setTooltext, [1, 2], {
                    formattedValue : x
                },
                D,
                h): g.useColorNameAsValue ? S: null === x?!1: x: !1,
                D = void 0 !== W ? W: D.label || (g.useColorNameAsValue ? S: x),
                I.toolText = B,
                I.displayValue = D,
                I.rolloverProperties = {
                    enabled: G,
                    hoverAttr: U,
                    hoverAnimAttr: void 0,
                    outAttr: P
                }
            }, init: function(a) {
                var c = this.chart;
                if (!a)return !1;
                this.JSONData = a;
                this.chartGraphics =
                c.chartGraphics;
                this.components = {};
                this.graphics = {};
                this.visible=!0;
                this.configure()
            }, updateData: function(a, c, b) {
                this._setConfigure(a, c);
                b && this.draw()
            }, draw: function() {
                var a = this.components.data, d = this.chart, r = this.config, l = d.getJobList(), n = d.get(S, c), H = n.animType, m = n.animObj, h = n.dummyObj, n = n.duration, w = d.config, q = w.canvasLeft, k = w.canvasTop, C = w.canvasHeight, x = w.canvasWidth, G = d.components.paper, D = d.graphics.datasetGroup, t = this.graphics.container, I = this.graphics.trackerContainer, K = d.graphics, J = K.trackerGroup,
                E, N = a[0], a = N && N.config, Z = w.gaugeoriginx; E = w.gaugeoriginy; var ua = w.gaugeradius, M = w.hasgaugeoriginx, Ba = w.hasgaugeoriginy, za = w.hasgaugeradius, fa = this.graphics.dataLabelContainer, d = d.config.dataLabelStyle, K = K.datalabelsGroup; t || (t = this.graphics.container = G.group("bulb", D));
                I || (this.graphics.trackerContainer = G.group("bulb-hot", J));
                fa || (fa = this.graphics.dataLabelContainer = G.group("datalabel"));
                K && K.appendChild(fa);
                I = N.trackerConfig = {};
                J = a.setValue;
                D = a.toolText;
                I.eventArgs = {
                    value: J,
                    displayValue: a.displayValue,
                    toolText: D ? D: ""
                };
                N.graphics || (N.graphics = {});
                Z = void 0 !== M ? Z * r.scaleFactor : q + x / 2;
                E = void 0 !== Ba ? E * r.scaleFactor : k + C / 2;
                k = Z;
                q = E;
                x = void 0 !== za ? ua * r.scaleFactor : y(x / 2, C / 2);
                w.gaugeStartX = Z - x;
                w.gaugeEndX = Z + x;
                w.gaugeStartY = E - x;
                w.gaugeEndY = E + x;
                w.gaugeCenterX = Z;
                w.gaugeCenterY = E;
                w.gaugeRadius = x;
                C = {
                    cx: k,
                    cy: q,
                    r: .001,
                    "stroke-linecap": f,
                    stroke: u(a.gaugeBorderColor),
                    "stroke-width": a.gaugeBorderThickness,
                    fill: u(a.fillColor),
                    ishot: !0
                };
                (E = N.graphics.element) ? (N.graphics.element.animateWith(h, m, {
                    cx: k,
                    cy: q,
                    r: x
                }, n, H), N.graphics.element.attr({
                    "stroke-linecap": f,
                    stroke: u(a.gaugeBorderColor),
                    "stroke-width": a.gaugeBorderThickness,
                    fill: u(a.fillColor),
                    ishot: !0
                })) : (E = N.graphics.element = G.circle(C, t), E.animateWith(h, m, {
                    r: x
                }, n, H));
                I.attr = {
                    cx: k,
                    cy: q,
                    r: x,
                    cursor: a.setLink ? "pointer": L,
                    stroke: W,
                    "stroke-width": a.plotBorderThickness,
                    fill: W,
                    ishot: !0,
                    visibility: B
                };
                t = N.graphics;
                w.placevaluesinside ? (w = q, N = g) : (w = q + x + w.valuepadding, N = F);
                a.setValue !== L && r.showValue ? (C = {
                    text: a.displayValue,
                    "text-anchor": g,
                    x: Z,
                    y: w,
                    "vertical-align": N,
                    fill: d.color,
                    direction: a.textDirection,
                    "text-bound": [d.backgroundColor,
                    d.borderColor, d.borderThickness, d.borderPadding, d.borderRadius, d.borderDash]
                }, t.label ? (t.label.show(), t.label.animateWith(h, m, {
                    x: Z,
                    y: w
                }, n, H), t.label.attr({
                    text: a.displayValue,
                    "text-anchor": g,
                    "vertical-align": N,
                    fill: d.color,
                    direction: a.textDirection,
                    "text-bound": [d.backgroundColor, d.borderColor, d.borderThickness, d.borderPadding, d.borderRadius, d.borderDash]
                })) : t.label = G.text(C, fa), t.label.tooltip(D)) : t.label && t.label.hide() && t.label.attr({
                    "text-bound": []
                });
                l.trackerDrawID.push(e.addJob(this.drawTracker.bind(this),
                b.priorityList.tracker))
            }, drawTracker: function() {
                var a = this.chart, c = a.config.plothovereffect, b = a.components.paper, f = this.graphics.trackerContainer, d, g, e, h, y, q = function(c) {
                    r.call(this, a, c)
                }, n = function(c) {
                    return function(b) {
                        var f = this.getData();
                        0 !== f.showHoverEffect&&!0 !== f.draged && (c.attr(this.getData().setRolloverAttr), r.call(this, a, b, "DataPlotRollOver"))
                    }
                }, k = function(c) {
                    return function(b) {
                        var f = this.getData();
                        0 !== f.showHoverEffect&&!0 !== f.draged && (c.attr(this.getData().setRolloutAttr), r.call(this,
                        a, b, "DataPlotRollOut"))
                    }
                };
                g = (e = this.components.data[0]) && e.config;
                d = e.trackerConfig;
                h = e.graphics.element;
                if (y = d.attr)
                    e.graphics.hotElement ? (e.graphics.hotElement.attr(y), b=!1) : (f = e.graphics.hotElement = b.circle(y, f), b=!0), f = e.graphics.hotElement, (f || h).data("eventArgs", d.eventArgs).data(H, c).data("setRolloverAttr", g.rolloverProperties.hoverAttr || {}).data("setRolloutAttr", g.rolloverProperties.outAttr || {}).tooltip(d.eventArgs.toolText), (b || g.elemCreated) && (f || h).click(q).hover(n(h), k(h))
            }, addData: function() {},
            removeData: function() {}
        }, void 0, {}
        ])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-dataset-progressgauge", function() {
        var b = this.hcLib, k = b.BLANKSTRING, d = b.pluck, a = b.getValidValue, n = b.pluckNumber, L = b.getFirstValue, D = b.graphics.getLightColor, u = b.graphics.convertColor, e = b.preDefStr, C = e.colors.FFFFFF, N = e.visibleStr, J = e.ROUND, K = b.parseUnsafeString, e = this.window, I = "rgba(192,192,192," + (/msie/i.test(e.navigator.userAgent)&&!e.opera ? .002 : 1E-6) + ")", e = Math, G = e.max, Z = e.min, S = b.regex.dropHash, c = b.HASHSTRING,
        F = b.toRaphaelColor, g = b.HUNDREDSTRING, H = b.COMMASPACE, B = b.COMMASTRING;
        E.register("component", ["dataset", "progressgauge", {
            type: "doughnut2d",
            configure: function() {
                var a = this.chart, c = this.config, e = this.JSONData, H = a.config, p = a.jsonData.chart, k = a.components.colorManager, v, l = c.plotColor = k.getPlotColor(this.index || this.positionIndex), u = n(e.dashed, p.plotborderdashed);
                n(p.useplotgradientcolor, 1);
                var F, m, h, w = b.getDashStyle, q = a.isBar, B = a.is3D, D = a.isStacked, x = {}, x = x.dataObj || (x.dataObj = {}), x = x.chart || (x.chart = {});
                c.showLegend = n(p.showlegend, 0);
                c.legendSymbolColor = c.plotColor;
                v = c.showplotborder = n(p.showplotborder, B ? 0 : 1);
                c.plotDashLen = F = n(p.plotborderdashlen, 5);
                c.plotDashGap = m = n(p.plotborderdashgap, 4);
                c.plotfillAngle = n(360 - p.plotfillangle, q ? 180 : 90);
                c.plotFillAlpha = h = d(e.alpha, p.plotfillalpha, "70");
                c.plotColor = d(e.color, l);
                c.isRoundEdges = l = n(p.useroundedges, 0);
                c.plotRadius = n(p.useRoundEdges, c.isRoundEdges ? 1 : 0);
                c.plotFillRatio = d(e.ratio, p.plotfillratio);
                c.plotgradientcolor = b.getDefinedColor(p.plotgradientcolor,
                k.getColor("plotGradientColor"));
                c.plotBorderAlpha = v ? d(p.plotborderalpha, h, g) : 0;
                c.plotBorderColor = d(p.plotbordercolor, B ? C : k.getColor("plotBorderColor"));
                c.plotBorderThickness = v = n(p.plotborderthickness, 1);
                c.plotBorderDashStyle = u ? w(F, m, v) : "none";
                c.showValues = n(e.showvalues, p.showvalues, 1);
                c.valuePadding = n(p.valuepadding, 2);
                c.enableAnimation = u = n(p.animation, p.defaultanimation, 1);
                c.animation = u ? {
                    duration: 1E3 * n(p.animationduration, 1)
                } : !1;
                x.transposeAnimation = c.transposeAnimation = n(p.transposeanimation,
                x.transposeAnimation, u);
                c.transposeAnimDuration = 700 * n(p.transposeanimduration, 1);
                c.showShadow = l || B ? n(p.showshadow, 1) : n(p.showshadow, k.getColor("showShadow"));
                c.showHoverEffect = n(p.plothovereffect, p.showhovereffect, void 0);
                c.showTooltip = n(p.showtooltip, 1);
                c.stack100Percent = a = n(a.stack100percent, p.stack100percent, 0);
                c.definedGroupPadding = G(n(p.plotspacepercent), 0);
                c.plotSpacePercent = G(n(p.plotspacepercent, 20)%100, 0);
                c.maxColWidth = n(q ? p.maxbarheight : p.maxcolwidth, 50);
                c.showPercentValues = n(p.showpercentvalues,
                D && a ? 1 : 0);
                c.showPercentInToolTip = n(p.showpercentintooltip, D && a ? 1 : 0);
                c.plotPaddingPercent = n(p.plotpaddingpercent);
                c.rotateValues = n(p.rotatevalues) ? 270 : 0;
                c.placeValuesInside = n(p.placevaluesinside, 0);
                c.zeroPlaneColor = H.zeroPlaneColor;
                c.zeroPlaneBorderColor = H.zeroPlaneBorderColor;
                c.zeroPlaneShowBorder = H.zeroPlaneShowBorder;
                c.use3DLighting = n(p.use3dlighting, 1);
                c.parentYAxis = "s" === d(e.parentyaxis && e.parentyaxis.toLowerCase(), "p") ? 1 : 0;
                this._setConfigure()
            },
            _setConfigure: function(c, g) {
                var e = this.chart, u =
                this.config, p = this.JSONData, C = c || p.data, v = C && C.length, l = e.config.categories, I = e.singleseries, l = l && l.length, v = c && c.data.length || Z(l, v), l = e.jsonData.chart, K = e.components.colorManager, m = u.showPlotBorder, h = u.plotColor = K.getPlotColor(this.index || this.positionIndex), w = n(l.showtooltip, 1), q = b.parseUnsafeString, z = q(l.yaxisname), A = q(l.xaxisname), x = q(d(l.tooltipsepchar, H)), J = n(l.seriesnameintooltip, 1), E = b.parseTooltext, t, N, S, R, Aa, Q, ya, ua = u.plotBorderThickness, M = u.isRoundEdges, Ba = u.showHoverEffect, za = u.plotFillAngle,
                fa, sa, ha, da = u.plotBorderDashStyle, X, ba, U, V, aa, la, oa, O, P, Y = b.getDashStyle, T = this.components.data, qa = e.isBar, wa = e.is3D, ja, ka = u.maxValue||-Infinity, Fa = u.minValue || Infinity, Ia = this.chart.components.numberFormatter, ea;
                T || (T = this.components.data = []);
                for (ja = 0; ja < v; ja++)
                    c ? (Q = c && c.data[ja], ea = void 0 !== g ? g + ja : T.length - v + ja, N = T[ea]) : (N = T[ja], Q = C[ja]), e = N && N.config, N || (N = T[ja] = {}), N.config || (e = T[ja].config = {}), e.visible = n(Q.visible, !0), e.showValue = n(Q.showvalue, u.showValues), e.setValue = N = Ia.getCleanValue(Q.value),
                    e.setLink = d(Q.link), e.toolTipValue = h = Ia.dataLabels(N, u.parentYAxis), e.setDisplayValue = R = q(Q.displayvalue), e.displayValue = d(R, h), h = n(Q.dashed), R = n(Q.dashlen, void 0), t = ya = n(Q.dashgap, ya), ka = G(ka, N), Fa = Z(Fa, N), e.plotBorderDashStyle = R = 1 === h ? Y(R, t, ua) : 0 === h ? "none" : da, I ? (h = K.getPlotColor(isNaN(ea) ? ja : ea), h = d(Q.color, h), sa = d(Q.ratio, u.plotFillRatio)) : h = d(Q.color, u.plotColor), fa = d(Q.alpha, u.plotFillAlpha), ha = d(Q.alpha, u.plotBorderAlpha, fa).toString(), e.plotColor = h, e.plotFillAlpha = d(Q.alpha, u.plotFillAlpha),
                    e.plotBackgroundFillAlpha = n(Q.plotbackgroundfillalpha, l.plotbackgroundfillalpha, 30), 0 > N&&!M && (Aa = za, za = qa ? 180 - za : 360 - za), e.colorArr = t = b.graphics.getColumnColor(h, fa, sa, za, M, u.plotBorderColor, ha, qa ? 1 : 0, wa?!0 : !1), N = a(q(d(Q.tooltext, Q.label))), 0 !== Ba && (X = d(Q.hovercolor, p.hovercolor, l.plotfillhovercolor, l.columnhovercolor, h), ba = d(Q.hoveralpha, p.hoveralpha, l.plotfillhoveralpha, l.columnhoveralpha, fa), U = d(Q.hovergradientcolor, p.hovergradientcolor, l.plothovergradientcolor, u.plotgradientcolor), !U && (U = k), V =
                    d(Q.hoverratio, p.hoverratio, l.plothoverratio, sa), aa = n(360 - Q.hoverangle, 360 - p.hoverangle, 360 - l.plothoverangle, za), la = d(Q.borderhovercolor, p.borderhovercolor, l.plotborderhovercolor, u.plotBorderColor), ha = d(Q.borderhoveralpha, p.borderhoveralpha, l.plotborderhoveralpha, ha, fa), fa = n(Q.borderhoverthickness, p.borderhoverthickness, l.plotborderhoverthickness, ua), oa = n(Q.borderhoverdashed, p.borderhoverdashed, l.plotborderhoverdashed), O = n(Q.borderhoverdashgap, p.borderhoverdashgap, l.plotborderhoverdashgap, void 0),
                    P = n(Q.borderhoverdashlen, p.borderhoverdashlen, l.plotborderhoverdashlen, ya), oa = oa ? Y(P, O, fa) : R, 1 == Ba && X === h && (X = D(X, 70)), h = b.graphics.getColumnColor(X + B + U, ba, V, aa, M, la, ha.toString(), qa ? 1 : 0, wa?!0 : !1), e.setRolloutAttr = {
                        fill: wa ? [F(t[0]), !u.use3DLighting]: F(t[0]),
                        stroke: m && F(t[1]),
                        "stroke-width": ua,
                        "stroke-dasharray": R
                    }, e.setRolloverAttr = {
                        fill: wa ? [F(h[0]), !u.use3DLighting]: F(h[0]),
                        stroke: m && F(h[1]),
                        "stroke-width": fa,
                        "stroke-dasharray": oa
                    }), t = e.toolTipValue, h = a(q(d(Q.tooltext, p.plottooltext, l.plottooltext))),
                    w ? null === t ? Q=!1 : void 0 !== h ? (R = [1, 2, 3, 4, 5, 6, 7], N = {
                        yaxisName : z, xaxisName : A, formattedValue : t, label : N
                },
                Q = E(h,
                R,
                N,
                Q,
                l,
                p)): (J && (S = L(p && p.seriesname)),
                Q = S ? S + x: k,
                Q += N ? N + x: k): Q=!1,
                e.toolText = Q,
                e.setTooltext = Q,
                Aa && (za = Aa),
                ea++;
                u.maxValue = ka;
                u.minValue = Fa
            }, init: function(a) {
                var c = this.chart;
                if (!a)return !1;
                this.JSONData = a;
                this.chartGraphics = c.chartGraphics;
                this.components = {};
                this.graphics = {};
                this.visible = 1 === n(this.JSONData.visible, !Number(this.JSONData.initiallyhidden), 1);
                this.configure();
                this.config.showLegend &&
                this._addLegend()
            }, _addLegend: function() {
                var a = this.chart, b = a.jsonData.chart, d = this.JSONData.data, g, e, u, v = this.components.data, l;
                for (e = 0;
                e < v.length;
                e += 1)u = v[e], g = u.config, l = d[e].label ? d[e].label : "Type " + (e + 1), u = {
                    FCcolor: {
                        color: g.plotColor,
                        angle: 0,
                        ratio: "0",
                        alpha: "100"
                    }
                }, g = D(g.plotColor, 60).replace(S, c), a.components.legend.addItems(this, this.legendInteractivity, {
                    type: this.type,
                    fillColor: F(u),
                    strokeColor: F(g),
                    enabled: n(b.includeinlegend, 1),
                    label: l,
                    index: e
                })
            }, legendInteractivity : function(a, c) {
                var b =
                a.components.data[c.configuration.index], d = a.config, g = b.graphics, e = b.config.visible, v = this.config, l = c.config, n = c.graphics, H = v.itemHiddenStyle.color, v = v.itemStyle.color, m = l.fillColor, l = l.strokeColor;
                b.config.visible = e ? 0 : 1;
                e ? (d.animation.duration && d.transposeAnimation || (g.gaugeBackground.hide(), g.gaugeMeter.hide(), g.hotElement.hide()), n.legendItemSymbol && n.legendItemSymbol.attr({
                    fill: H,
                    stroke: H
                }), n.legendItemText && n.legendItemText.attr({
                    fill: u(H)
                }), n.legendIconLine && n.legendIconLine.attr({
                    stroke: H
                })) :
                (g.gaugeBackground.show(), g.gaugeMeter.show(), g.hotElement.show(), n.legendItemSymbol && n.legendItemSymbol.attr({
                    fill: m,
                    stroke: l
                }), n.legendItemText && n.legendItemText.attr({
                    fill: u(v)
                }), n.legendIconLine && n.legendIconLine.attr({
                    stroke: m
                }));
                a.draw()
            }, draw: function() {
                var c = this, b = c.JSONData, g = c.config, e = c.chart.jsonData.chart, p = c.components.data, n = c.components.data, v = c.chart, l = v.components.paper, H = v.config, B = H.canvasLeft, m = H.canvasTop, h = H.canvasHeight, w = H.canvasWidth, q = v.graphics.datasetGroup, H = c.graphics.gaugeBackgroundContainer,
                C = c.graphics.gaugeMeterContainer, D = c.graphics.trackerContainer, x = v.graphics.trackerGroup, G = c.components.data.length || 0, E, t, L, S, R, Aa, Q, ya, ua, M, Ba, za = g.animation.duration, fa = g.transposeAnimation, g = g.transposeAnimDuration;
                Aa = 0;
                var sa = function() {
                    var a = c.components.data, b, d = c.components.data.length, g;
                    for (E = 0; E < d; E++)
                        b = a[E].config, g = a[E].graphics, b.visible || g.hotElement.hide()
                };
                H || (H = c.graphics.gaugeBackgroundContainer = l.group("gauge-background", q));
                C || (C = c.graphics.gaugeMeterContainer = l.group("gauge-meter",
                q));
                D || (D = c.graphics.trackerContainer = l.group("meter-hot", x));
                for (E = 0; E < G; E++)
                    t = p[E], L = (q = n[E]) && q.config, L.visible && (Aa += 1);
                q = Z(h / 2, w / 2);
                v = x = .2 * Z(v.config.canvasWidth, v.config.canvasHeight);
                Aa = (q - Aa - v) / Aa;
                B += .5 * w;
                m += .5 * h;
                for (E = 0; E < G; E++)
                    if (t = p[E], L = (q = n[E]) && q.config, (w = L.visible) || za)
                        q.graphics || (n[E].graphics = {}), S = L.setValue, h = L.setLink, t = a(K(d(t.tooltext, b.plottooltext, e.plottooltext))), t = L.toolText + (t ? k : L.toolTipValue), R = L.colorArr, Ba = E, M = {
                            index: E,
                            link: h,
                            value: S,
                            displayValue: L.displayValue,
                            toolText: t,
                            id: k,
                            datasetIndex: c.datasetIndex,
                            datasetName: b.seriesname,
                            dataSet: c,
                            visible: N,
                            cx: B,
                            cy: m,
                            radius: x
                        }, Q = v, ya = v + Aa, ua = 6.283, q.graphics.gaugeBackground ? (L = {
                            ringpath: [B, m, Q, ya, 0, ua],
                            fill: u(L.plotColor, L.plotBackgroundFillAlpha),
                            "stroke-width": 0
                        }, fa ? (L = w ? [B, m, Q, ya, 0, ua] : [B, m, Q, Q, 0, ua], q.graphics.gaugeBackground.animate({
                            ringpath: L
                        }, g, "easeIn")) : q.graphics.gaugeBackground.attr(L)) : q.graphics.gaugeBackground = l.ringpath(B, m, Q, ya, 0, ua, H).attr({
                            fill: u(L.plotColor, L.plotBackgroundFillAlpha),
                            "stroke-width": 0
                        }),
                        ua = Z(3.6 * S, 359.99), Q = [B, m, v + Aa / 2, Z(3.6 * S, 359.99)], za && (ua = 0), q.graphics.gaugeMeter ? (L = {
                            arcpath: Q,
                            stroke: F(R[0]),
                            "stroke-width": Aa,
                            "stroke-linecap": J
                        }, fa ? w ? q.graphics.gaugeMeter.animate({
                            arcpath: Q,
                            "stroke-width": Aa
                        }, g, "easeIn", sa) : (Q = [B, m, v, 0], q.graphics.gaugeMeter.animate({
                            arcpath: Q,
                            "stroke-width": 0
                        }, g, "easeIn", sa)) : q.graphics.gaugeMeter.attr(L)) : (q.graphics.gaugeMeter = l.arcpath(B, m, v + Aa / 2, ua, C).attr({
                            stroke: F(R[0]),
                            "stroke-width": Aa,
                            "stroke-linecap": J
                        }), za && q.graphics.gaugeMeter.animate({
                            arcpath: Q
                        },
                        za, "easeIn")), L = {
                            arcpath: Q,
                            cursor: h ? "pointer": k,
                            stroke: I,
                            "stroke-width": Aa,
                            "stroke-linecap": J,
                            ishot: !0
                        }, q.graphics.hotElement ? q.graphics.hotElement.attr(L) : (L = q.graphics.hotElement = l.arcpath(B, m, v + Aa / 2, Z(3.6 * S, 359.99), D), q.graphics.hotElement.attr({
                            cursor: h ? "pointer": k,
                            stroke: I,
                            "stroke-width": Aa,
                            "stroke-linecap": J,
                            ishot: !0
                        })), L = q.graphics.hotElement, L.data("eventArgs", M).data("groupId", Ba).hover(c._plotRollOver, c._plotRollOut).tooltip(t), w && (v += Aa + 1)
            }, _drawDoughnutCenterLabel: function(a, c, b, d, g, e,
            n, l, u) {
                var H = this.chart;
                c = this.config;
                var m = e || c.lastCenterLabelConfig;
                e = H.components.paper;
                var h = H.linkedItems.smartLabel;
                b = this.components.data[u.index];
                n = {
                    fontFamily: "Verdana",
                    fontSize: "12px",
                    lineHeight: 1.2 * 12 + "px",
                    fontWeight: "bold",
                    fontStyle: k
                };
                d = 1.414 * (.5 * d - 2) - 4;
                var w = 1.414 * (.5 * g - 2) - 4, q = H.graphics.datalabelsGroup;
                g = this.graphics.dataLabelContainer;
                h.useEllipsesOnOverflow(H.config.useEllipsesWhenOverflow);
                h.setStyle(n);
                a = h.getSmartText(a, d, w);
                g || (g = this.graphics.dataLabelContainer = e.group("datalabel",
                q));
                u = {
                    x: u.cx,
                    y: u.cy,
                    text: a.text,
                    direction: c.textDirection,
                    opacity: l ? 0: 1,
                    fill: F(b.config.colorArr[0]),
                    "text-bound": m.bgOval ? "none": [F({
                        color: m.bgColor,
                        alpha: m.bgAlpha
                    }), F({
                        color: m.borderColor,
                        alpha: m.borderAlpha
                    }), m.borderThickness, m.textPadding, m.borderRadius]
                };
                b.graphics.label || (b.graphics.label = e.text(u, g));
                b.graphics.label.attr(u).css(n);
                l ? b.graphics.label.animate({
                    opacity: 1
                }, 100, "easeIn") : b.graphics.label.animate({
                    opacity: 0
                }, 100, "easeOut")
            }, _plotRollOver: function() {
                var a = this.data("eventArgs"),
                c = 2 * a.radius;
                a.dataSet._drawDoughnutCenterLabel(a.toolText, 300, 196, c, c, a.toolText, !1, !0, a)
            }, _plotRollOut: function() {
                var a = this.data("eventArgs"), c = 2 * a.radius;
                a.dataSet._drawDoughnutCenterLabel(a.toolText, 300, 196, c, c, a.toolText, !1, !1, a)
            }, updateData: function(a, c, b) {
                var d = this.config, g = d.maxValue, e = d.prevMin, n = this.chart;
                this._setConfigure(a, c);
                this.setMaxMin();
                if (d.maxValue !== g || d.minValue !== e)
                    this.maxminFlag=!0;
                b && (n._setAxisLimits(), this.draw())
            }, removeData: function(a, c, b) {
                var d = this.components, g =
                d.data, e = d.removeDataArr || (d.removeDataArr = []), n = this.config, l = this.maxminFlag;
                c = c || 1;
                a = a || 0;
                if (a + c === g.length)
                    this.endPosition=!0;
                else if (0 === a || void 0 === a)
                    this.endPosition=!1;
                g[a] && (g[a].config.visible = 0);
                b && this.draw();
                d.removeDataArr = e = e.concat(g.splice(a, c));
                b = e.length;
                for (a = 0; a < b; a++)
                    if (e[a]) {
                        c = e[a].config;
                        if (c.setValue === n.maxValue || c.setValue === n.minValue)
                            l = this.maxminFlag=!0;
                            if (l)
                                break
                    }
                l && this.setMaxMin()
            }, manageSpace: function() {}
        }, "Column"])
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-realtimecolumn",
    function() {
        var b = this, k = b.hcLib, d=!k.CREDIT_REGEX.test(b.window.location.hostname), a = k.BLANKSTRING, n = k.COMMASPACE, E = Math.max, D = k.regex.dropHash, u = k.HASHSTRING, e = k.graphics.convertColor, C = k.preDefStr, N = C.animationObjStr, J = C.visibleStr, K = C.configStr, I = k.pluck, G = k.pluckNumber, Z = k.schedular, S = k.priorityList, C = k.chartAPI;
        C("realtimecolumn", {
            showRTvalue: !0,
            canvasPadding: !0,
            isRealTime: !0,
            standaloneInit: !0,
            creditLabel: d,
            defaultDatasetType: "realtimecolumn",
            applicableDSList: {
                realtimecolumn: !0
            },
            transposeAxis: !0,
            rtManageSpace: !0,
            _realTimeConfigure: function() {
                var c = this.config, b = c.animationObj, c = c.realTimeConfig || (c.realTimeConfig = {}), d, e, k, f;
                d = this.jsonData;
                var r = d.chart;
                f = d.categories && d.categories[0] && d.categories[0].category && d.categories[0].category.length || 0;
                c.showRTValue = G(r.showrealtimevalue, 1);
                c.dataStreamURL = I(r.datastreamurl, a);
                c.dataStamp = r.datastamp;
                c.useMessageLog = G(r.usemessagelog, 0);
                c.clearInterval = G(r.clearchartinterval, 0);
                c.realtimeValueSeparator = I(r.realtimevaluesep, n);
                c.refreshInterval =
                d = G(r.refreshinterval, r.updateinterval, 2);
                c.updateInterval = this.config.updateInterval = G(r.updateinterval, d);
                c.realtimeValuePadding = G(r.realtimevaluepadding);
                c.realtimeValueFont = I(r.realtimevaluefont, a);
                c.realtimeValueFontBold = I(r.realtimevaluefontbold, 0);
                c.realtimeValueFontColor = e = I(r.realtimevaluefontcolor, a);
                c.realtimeValueFontSize = k = G(r.realtimevaluefontsize, a);
                c.realTimeValuePadding = G(r.realtimevaluepadding, 5);
                c.fontWeight = G(r.realtimevaluefontbold, 0) ? "bold" : "normal";
                c.numDisplaySets = G(r.numdisplaysets,
                E(f, 15));
                c.refreshInstantly = G(r.refreshinstantly, 0);
                c.showRTmenuItem = f = G(r.showrtmenuitem, 0);
                c.sync = G(r.sync, .6 < d?!0 : !1);
                e && (c.realtimeValueFontColor = e.replace(D, u));
                k && (c.realtimeValueFontSize = k + "px");
                d*=1E3;
                b.duration > d && (b.duration = d);
                f && this._setRTmenu()
            },
            _setRealTimeCategories: function() {
                var a = this.components.xAxis[0], b = [], d = this.config.realTimeConfig, e = d && d.clear ? void 0: this.jsonData.categories && this.jsonData.categories[0] && this.jsonData.categories[0].category, n = a.getCategoryLen(), d = d.numDisplaySets,
                n = a.getCategoryLen();
                n < d ? (b.length = d - n, e = e ? b.concat(e) : b, a.setCategory(e)) : n > d && (e.splice(d, n - d), a.setCategory(e))
            },
            _realTimeValuePositioning: function(a) {
                var b = this.components, d = this.linkedItems.smartLabel, n;
                n = this.config;
                var u = n.realTimeConfig || (n.realTimeConfig = {}), f = u.realTimeValuePadding, b = b.xAxis[0].config, r = b.trend.trendStyle, b = u.style = {
                    color: e(I(u.realtimeValueFontColor, r.color), I(b.trendlineAlpha, 99)),
                    fontFamily: I(u.realtimeValueFont, r.fontFamily),
                    fontSize: I(u.realtimeValueFontSize, r.fontSize),
                    fontWeight: I(u.fontWeight, r.fontWeight),
                    lineHeight: G(r.lineHeight)
                };
                d.useEllipsesOnOverflow(n.useEllipsesWhenOverflow);
                d.setStyle(b);
                u.height = d = d.getOriSize(k.TESTSTR).height;
                u.canvasBottom = n.canvasBottom;
                n = d + f;
                n > a && (n = a);
                return {
                    bottom: n
                }
            },
            _drawRealTimeValue: function() {
                var c = this.components, b = this.config, d = c.dataset, e = c.paper, n = this.linkedItems.smartLabel, f = b.realTimeConfig, r = f.realtimeValueSeparator, u = d.length, k = a, p = this.get(K, N), C = p.animObj, v = p.dummyObj, p = p.duration, l = f.canvasBottom, D = f.height,
                G = b.canvasLeft, m = b.canvasRight, h = f.style || {}, c = c.realTimeValue || (c.realTimeValue = {}), w = c.graphics, q = this.graphics, z = q.parentGroup, A = q.realTimeValueGroup, x;
                f.clear && c.graphics && c.graphics.attr({
                    text: a
                });
                if (A) {
                    for (f = 0; f < u; f++)
                        q = d[f].components.data, q = (q = q[q.length - 1]) && q.config.displayValue, k += q ? void 0 === q ? a : q + r : a;
                    k = k.substring(0, k.length - r.length);
                    n.useEllipsesOnOverflow(b.useEllipsesWhenOverflow);
                    n.setStyle(h);
                    n.getOriSize(k);
                    b = {
                        x: (G + m) / 2 || 0,
                        y: l - D / 2 || 0,
                        "font-size": h.fontSize,
                        "font-weight": h.fontWeight,
                        "font-family": h.fontFamily,
                        "line-height": h.lineHeight,
                        visibility: J
                    };
                    w || (w = c.graphics = e.text(b, A), x=!0);
                    w.attr({
                        text: k,
                        fill: h.color
                    });
                    w&&!x && (w.show(), w.animateWith(v, C, b, p))
                } else 
                    q.realTimeValueGroup = e.group("realTimeValue", z).insertBefore(q.datalabelsGroup)
            },
            _hideRealTimeValue: function() {
                var a = this.components;
                (a = (a = a.realTimeValue || (a.realTimeValue = {}), a.graphics)) && a.hide()
            },
            _getData: function() {
                var a = this.components, b = a.dataset, a = a.xAxis && a.xAxis[0] || a.scale, d, e, n, f = [], r = 0, u, k;
                if (b) {
                    d = b.length;
                    for (e = 0; e < d; e++)
                        r = E(r, b[e].components.data.length);
                    for (e = 0; e < r; e++)
                        for (k = f[e] = [], k[0] = a.getLabel(e).label, n = 1; n <= d; n++)
                            u = b[n - 1].components.data[e], k[n] = u && u.config.setValue;
                    return f
                }
            },
            _setData: function(c, b) {
                var d = a;
                if (c && c.toString || c === a || 0 === c)
                    d = "value=" + c.toString();
                if (b && b.toString || b === a)
                    d = d + "&label=" + b.toString();
                d && this.feedData(d)
            },
            _stopUpdate: function(a) {
                var d = this.config.realTimeConfig, g = this.linkedItems.timers && this.linkedItems.timers.setTimeout.loadData, e = this.chartInstance, n = e.__state;
                clearTimeout(n._toRealtime);
                n._rtAjaxObj && n._rtAjaxObj.abort();
                n._rtPaused=!0;
                g && clearTimeout(g);
                d.clearIntervalFlag=!1;
                b.raiseEvent("realimeUpdateStopped", {
                    source: a
                }, e)
            },
            _restartUpdate: function() {
                var a = this.chartInstance.__state;
                a._rtDataUrl && a._rtPaused && (a._rtPaused=!1, a._rtAjaxObj.get(a._rtDataUrl))
            },
            _isUpdateActive: function() {
                return !this.chartInstance.__state._rtPaused
            },
            _setRTmenu: function() {
                var a = this, b = a.chartMenuTools, d = b.set, e = b.update, n=!1;
                d([{
                    "Clear Chart": {
                        handler: function() {
                            a._clearChart.call(a)
                        },
                        action: "click"
                    }
                }, {
                    "Stop Update": {
                        handler: function() {
                            n ? (a._restartUpdate.call(a), e("Restart Update", "Stop Update", a), n=!1) : (a._stopUpdate.call(a), e("Stop Update", "Restart Update", a), n=!0)
                        },
                        action: "click"
                    }
                }
                ])
            },
            _getDataJSON: function() {
                return this.config.realTimeConfig.legacyUpdateObj || {
                    values: []
                }
            },
            _setRTdata: function() {
                var a = this.components.dataset, b, d, e, n, f, r;
                for (b = a.length; b--;)
                    e = a[b], e = e.components, d = e.removeDataArr || [], n = d.length, f = e.data, r = f.length, d = [].concat(d, f.slice(n, r)), e.dataRT = d
            },
            eiMethods: {
                feedData: function() {
                    var a =
                    this.apiInstance, b = a.getJobList(), d = arguments[1], e, n;
                    if (a.chartInstance.args.asyncRender || d)
                        e = arguments[0], b.eiMethods.push(Z.addJob(function() {
                            n = a.feedData.call(a, e);
                            "function" === typeof d && d(n)
                        }, S.postRender));
                    else 
                        return a.feedData.apply(a, arguments)
                },
                setData: function() {
                    var a = this.apiInstance, b = a.getJobList(), d = arguments;
                    a.chartInstance.args.asyncRender ? b.eiMethods.push(Z.addJob(function() {
                        a._setData.apply(a, d)
                    }, S.postRender)) : a._setData.apply(a, d)
                },
                stopUpdate: function() {
                    var a = this.apiInstance,
                    b = a.getJobList(), d = arguments;
                    a.chartInstance.args.asyncRender ? b.eiMethods.push(Z.addJob(function() {
                        a._stopUpdate.apply(a, d)
                    }, S.postRender)) : a._stopUpdate.apply(a, d)
                },
                restartUpdate: function() {
                    this.apiInstance._restartUpdate.apply(this.apiInstance, arguments)
                },
                isUpdateActive: function() {
                    return this.apiInstance._isUpdateActive.apply(this.apiInstance, arguments)
                },
                clearChart: function() {
                    var a = this.apiInstance, b = a.getJobList(), d = arguments;
                    a.chartInstance.args.asyncRender ? b.eiMethods.push(Z.addJob(function() {
                        a._clearChart.apply(a,
                        d)
                    }, S.postRender)) : a._clearChart.apply(a, d)
                },
                getData: function() {
                    return this.apiInstance._getData.apply(this.apiInstance, arguments)
                },
                showLog: function() {
                    var a = this.apiInstance.components && this.apiInstance.components.messageLogger;
                    return a && a.show && a.show.apply(a, arguments)
                },
                hideLog: function() {
                    var a = this.apiInstance.components && this.apiInstance.components.messageLogger;
                    return a && a.hide && a.hide.apply(a, arguments)
                },
                clearLog: function() {
                    var a = this.apiInstance.components && this.apiInstance.components.messageLogger;
                    return a && a.clearLog && a.clearLog.apply(a, arguments)
                },
                getDataForId: function() {
                    return this.apiInstance._getDataForId.apply(this.apiInstance, arguments)
                },
                setDataForId: function() {
                    return this.apiInstance._setDataForId.apply(this.apiInstance, arguments)
                },
                getDataJSON: function() {
                    return this.apiInstance._getDataJSON.apply(this.apiInstance, arguments)
                }
            }
        }, C.mscartesian, {
            enablemousetracking: !0
        })
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-realtimestackedcolumn", function() {
        var b = this.hcLib.chartAPI;
        b("realtimestackedcolumn",
        {}, b.realtimecolumn, {
            isstacked: !0,
            enablemousetracking: !0
        })
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-gaugebase", function() {
        var b = this.hcLib, k=!b.CREDIT_REGEX.test(this.window.location.hostname), d = b.chartAPI, a = b.extend2, b = a({}, b.defaultGaugePaletteOptions);
        d("gaugebase", {
            creaditLabel: k,
            defaultPaletteOptions: b,
            multiValueGauge: !1,
            decimals: 2,
            formatnumberscale: 0,
            drawAnnotations: !0,
            useScaleRecursively: !0,
            includeColorRangeInLimits: !1,
            isWidget: !0,
            _createAxes: function() {},
            _feedAxesRawData: function() {},
            _setCategories: function() {},
            _setAxisLimits: function() {},
            realTimeUpdate: function(a) {
                var b = this.components.dataset, d = a.dataset;
                a = a.categories && a.categories.category || [];
                var u = this.config.realTimeConfig = this.config.realTimeConfig || (this.config.realTimeConfig = {}), e, k, E;
                if (b) {
                    E = d[0];
                    d = E.data;
                    k = 0;
                    for (e = d.length; k < e; k++)
                        d[k].label = a[k] && a[k].label;
                    b[0].updateData(E);
                    b[0].maxminFlag && (u.maxminFlag = b[0].maxminFlag)
                }
            },
            _clearChart: function() {},
            _realTimeConfigure: d.realtimecolumn,
            _setRTmenu: function() {
                var a =
                this, b = a.chartMenuTools, d = b.set, u = b.update, e=!1;
                d([{
                    "Stop Update": {
                        handler: function() {
                            e ? (a._restartUpdate.call(a), u("Restart Update", "Stop Update", a), e=!1) : (a._stopUpdate.call(a), u("Stop Update", "Restart Update", a), e=!0)
                        },
                        action: "click"
                    }
                }
                ])
            },
            _getData: d.realtimecolumn,
            _setData: d.realtimecolumn,
            _stopUpdate: d.realtimecolumn,
            _restartUpdate: d.realtimecolumn,
            _isUpdateActive: d.realtimecolumn,
            eiMethods: d.realtimecolumn
        }, d.sscartesian, {
            valuefontbold: 1
        })
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-axisgaugebase",
    function() {
        var b = this.hcLib, k = b.chartAPI, d = b.pluck, a = b.pluckNumber, n = b.BLANKSTRING, L = b.pluckFontSize;
        k("axisgaugebase", {
            chartLeftMargin: 15,
            chartRightMargin: 15,
            chartTopMargin: 10,
            chartBottomMargin: 10,
            minChartHeight: 10,
            minCanvasWidth: 0,
            annotationRelativeLayer: "axis",
            _createAxes: function() {
                var a = this.components, b = E.register("component", ["axis", "gauge"]);
                a.scale = a = new b;
                a.chart = this;
                a.init()
            },
            _feedAxesRawData: function() {
                var n = this.components, u = n.colorManager, e = this.jsonData, k = e.chart, E = b.chartPaletteStr.chart2D,
                J = a(k.ticksbelowgauge), K = a(k.ticksonright), J = a(k.axisontop, void 0 !== k.axisonleft?!a(k.axisonleft) : void 0, void 0 !== J?!J : void 0, void 0 !== K ? K : void 0, this.isAxisOpposite), K = a(k.reverseaxis, this.isAxisReverse), I = a(k.showtickmarks, 1), G = a(k.showtickvalues), I = G || void 0 === G ? I || void 0 !== G?!0 : !1 : !1, u = {
                    outCanfontFamily: d(k.outcnvbasefont, k.basefont, "Verdana,sans"),
                    outCanfontSize: L(k.outcnvbasefontsize, k.basefontsize, 10),
                    outCancolor: d(k.outcnvbasefontcolor, k.basefontcolor, u.getColor(E.baseFontColor)).replace(/^#?([a-f0-9]+)/ig,
                    "#$1"),
                    useEllipsesWhenOverflow: k.useellipseswhenoverflow,
                    divLineColor: d(k.vdivlinecolor, u.getColor(E.divLineColor)),
                    divLineAlpha: d(k.vdivlinealpha, u.getColor("divLineAlpha")),
                    divLineThickness: a(k.vdivlinethickness, 1),
                    divLineIsDashed: !!a(k.vdivlinedashed, k.vdivlineisdashed, 0),
                    divLineDashLen: a(k.vdivlinedashlen, 4),
                    divLineDashGap: a(k.vdivlinedashgap, 2),
                    showAlternateGridColor: a(k.showalternatevgridcolor, 0),
                    alternateGridColor: d(k.alternatevgridcolor, u.getColor("altVGridColor")),
                    alternateGridAlpha: d(k.alternatevgridalpha,
                    u.getColor("altVGridAlpha")),
                    numDivLines: k.numvdivlines,
                    labelFont: k.labelfont,
                    labelFontSize: k.labelfontsize,
                    labelFontColor: k.labelfontcolor,
                    labelFontAlpha: k.labelalpha,
                    labelFontBold: k.labelfontbold,
                    labelFontItalic: k.labelfontitalic,
                    axisName: k.xaxisname,
                    axisMinValue: k.lowerlimit,
                    axisMaxValue: k.upperlimit,
                    setAdaptiveMin: k.setadaptivemin,
                    adjustDiv: k.adjustvdiv,
                    labelDisplay: k.labeldisplay,
                    showLabels: k.showlabels,
                    rotateLabels: k.rotatelabels,
                    slantLabel: a(k.slantlabels, k.slantlabel),
                    labelStep: a(k.labelstep,
                    k.xaxisvaluesstep),
                    showAxisValues: a(k.showxaxisvalues, k.showxaxisvalue),
                    showDivLineValues: a(k.showvdivlinevalues, k.showvdivlinevalues),
                    showZeroPlane: k.showvzeroplane,
                    zeroPlaneColor: k.vzeroplanecolor,
                    zeroPlaneThickness: k.vzeroplanethickness,
                    zeroPlaneAlpha: k.vzeroplanealpha,
                    showZeroPlaneValue: k.showvzeroplanevalue,
                    trendlineColor: k.trendlinecolor,
                    trendlineToolText: k.trendlinetooltext,
                    trendlineThickness: k.trendlinethickness,
                    trendlineAlpha: k.trendlinealpha,
                    showTrendlinesOnTop: k.showtrendlinesontop,
                    showAxisLine: a(k.showxaxisline,
                    k.showaxislines, k.drawAxisLines, 0),
                    axisLineThickness: a(k.xaxislinethickness, k.axislinethickness, 1),
                    axisLineAlpha: a(k.xaxislinealpha, k.axislinealpha, 100),
                    axisLineColor: d(k.xaxislinecolor, k.axislinecolor, "#000000"),
                    majorTMNumber: k.majortmnumber,
                    majorTMColor: k.majortmcolor,
                    majorTMAlpha: k.majortmalpha,
                    majorTMHeight: k.majortmheight,
                    tickValueStep: k.tickvaluestep,
                    showTickMarks: k.showtickmarks,
                    connectTickMarks: k.connecttickmarks,
                    showTickValues: k.showtickvalues,
                    majorTMThickness: k.majortmthickness,
                    upperlimit: n.numberFormatter.getCleanValue(k.upperlimit),
                    lowerlimit: n.numberFormatter.getCleanValue(k.lowerlimit),
                    reverseScale: k.reversescale,
                    showLimits: k.showlimits || I,
                    adjustTM: k.adjusttm,
                    minorTMNumber: a(k.minortmnumber, this.minorTMNumber, 4),
                    minorTMColor: k.minortmcolor,
                    minorTMAlpha: k.minortmalpha,
                    minorTMHeight: a(k.minortmheight, k.minortmwidth),
                    minorTMThickness: k.minortmthickness,
                    tickMarkDistance: a(k.tickmarkdistance, k.tickmarkgap),
                    tickValueDistance: a(k.tickvaluedistance, k.displayvaluedistance),
                    placeTicksInside: k.placeticksinside,
                    placeValuesInside: k.placevaluesinside,
                    upperLimitDisplay: k.upperlimitdisplay,
                    lowerLimitDisplay: k.lowerlimitdisplay,
                    drawTickMarkConnector: this.isHorizontal ? 1: 0
                }, n = n.scale;
                n.vtrendlines = e.trendpoints;
                n.chart = this;
                n.setCommonConfigArr(u, !this.isHorizontal, K, J);
                n.configure()
            },
            _setAxisLimits: function() {
                var b = this.components, d = this.jsonData.chart, e = b.scale, b = b.dataset[0].getDataLimits();
                - Infinity === b.max && (b.max = 0);
                Infinity === b.min && (b.min = 0);
                this.colorRange && e.setAxisConfig({
                    lowerlimit: a(d.lowerlimit, b.forceMin ? b.min : void 0),
                    upperlimit: a(d.upperlimit,
                    b.forceMax ? b.max : void 0)
                });
                e.setDataLimit(b.max, b.min)
            },
            _spaceManager: function() {
                var b, d = this.config, e = this.components, k = e.dataset[0], e = e.scale, n = this.jsonData.chart, E = a(n.showborder, this.is3D ? 0 : 1), K = this.isHorizontal, I = d.minChartWidth, G = d.minChartHeight, n = d.borderWidth = E ? a(n.borderthickness, 1): 0, L;
                d.canvasWidth - 2 * n < I && (b = (d.canvasWidth - I) / 2);
                d.canvasHeight - 2 * n < G && (L = (d.canvasHeight - G) / 2);
                this._allocateSpace({
                    top: L || n,
                    bottom: L || n,
                    left: b || n,
                    right: b || n
                });
                b = .225 * d.availableHeight;
                b = this._manageActionBarSpace &&
                this._manageActionBarSpace(b) || {};
                this._allocateSpace(b);
                K ? this._allocateSpace(e.placeAxis(d.availableHeight)) : this._allocateSpace(e.placeAxis(d.availableWidth));
                this._manageChartMenuBar(.4 * d.availableHeight);
                k._manageSpace && this._allocateSpace(k._manageSpace(d.availableHeight));
                e.setAxisConfig({
                    drawPlotlines: this.drawPlotlines,
                    drawPlotBands: this.drawPlotBands
                })
            },
            _postSpaceManagement: function() {
                var a = this.config, b = this.components.scale;
                this.isHorizontal ? b.setAxisDimention({
                    axisLength: a.canvasWidth
                }) :
                b.setAxisDimention({
                    axisLength: a.canvasHeight
                })
            },
            _getDataJSON: function() {
                var a = 0, b, d, k = [], E = [], L = [], K = this.components.dataset[0].components.data;
                for (b = K && K.length ? K.length : 0; a < b; a += 1)
                    d = K[a].config, k.push(d.itemValue), E.push(d.formatedVal || n), L.push(d.toolText || n);
                return {
                    values: k,
                    labels: E,
                    toolTexts: L
                }
            }
        }, k.gaugebase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-funnelpyramidbase", function() {
        var b = this.hcLib, k = b.chartAPI, d = b.preDefStr, a = b.Raphael, n = d.configStr, E = d.ROUND, D = d.animationObjStr;
        k("funnelpyramidbase", {
            showRTvalue: !1,
            canvasPadding: !1,
            sliceOnLegendClick: !0,
            defaultDatasetType: "funnelpyramidbaseds",
            applicableDSList: {
                funnel: !0
            },
            hasCanvas: !1,
            defaultPlotShadow: 1,
            subTitleFontSizeExtender: 0,
            tooltippadding: 3,
            defaultPaletteOptions: b.defaultPaletteOptions,
            drawAnnotations: !0,
            hasLegend: !0,
            isDataLabelBold: !1,
            dontShowLegendByDefault: !0,
            formatnumberscale: 1,
            isSingleSeries: !0,
            alignCaptionWithCanvas: 0,
            _updateVisuals: function() {
                var b = this.config, d = this.linkedItems.container, k = this.components,
                N = k.legend, J = k.paper, K = k.tooltip, I = this.chartInstance, G = this.get(n, D), Z = G.animType, S = G.dummyObj, c = G.animObj, G = G.duration, F;
                b.animationStarted=!0;
                J ? (b = {
                    width: d.offsetWidth,
                    height: d.offsetHeight
                }, F=!0, this._chartAnimation(!0), J.animateWith(S, c, b, G, Z)) : (J = k.paper = new a(d, d.offsetWidth, d.offsetHeight), J.setConfig("stroke-linecap", E));
                a.svg && this._createDummyText();
                J.tooltip(K.style, K.config.shadow, K.config.constrain);
                this.setChartCursor();
                this._createLayers();
                this._setDataLabelStyle();
                !F && this._chartAnimation(!0);
                this._drawBackground();
                k.chartMenuBar && this._drawChartMenuBar();
                this._manageCaptionPosition();
                k.caption && k.caption.draw();
                k.actionBar && this.drawActionBar();
                this._drawDataset();
                !1 !== this.hasLegend && N.drawLegend();
                this._drawCreditLabel();
                this._drawLogo();
                I.annotations && this._drawAnnotations();
                this.createChartStyleSheet()
            }
        }, k.gaugebase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-funnel", function() {
        var b = this.hcLib, k=!b.CREDIT_REGEX.test(this.window.location.hostname), b = b.chartAPI;
        b("funnel",
        {
            friendlyName: "Funnel Chart",
            standaloneInit: !0,
            creditLabel: k,
            defaultDatasetType: "funnel",
            applicableDSList: {
                funnel: !0
            },
            useSortedData: !0
        }, b.funnelpyramidbase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-pyramid", function() {
        var b = this.hcLib, k=!b.CREDIT_REGEX.test(this.window.location.hostname), b = b.chartAPI;
        b("pyramid", {
            friendlyName: "Funnel Chart",
            standaloneInit: !0,
            creditLabel: k,
            defaultDatasetType: "pyramid",
            applicableDSList: {
                pyramid: !0
            },
            useSortedData: !1
        }, b.funnelpyramidbase)
    }
    ]);
    E.register("module",
    ["private", "modules.renderer.js-vled", function() {
        var b = this.hcLib, k = b.pluckNumber, d=!b.CREDIT_REGEX.test(this.window.location.hostname), b = b.chartAPI;
        b("vled", {
            showRTvalue: !1,
            canvasPadding: !1,
            friendlyName: "Vertical LED Gauge",
            defaultSeriesType: "led",
            defaultPlotShadow: 1,
            standaloneInit: !0,
            realtimeEnabled: !0,
            chartleftmargin: 15,
            chartrightmargin: 15,
            charttopmargin: 10,
            chartbottommargin: 10,
            showTooltip: 0,
            connectTickMarks: 0,
            creditLabel: d,
            isHorizontal: !1,
            isAxisOpposite: !0,
            hasLegend: !1,
            drawPlotlines: !1,
            drawPlotBands: !1,
            isAxisReverse: !1,
            hasCanvas: !1,
            isRealTime: !0,
            defaultDatasetType: "led",
            colorRange: !0,
            applicableDSList: {
                led: !0
            },
            _getData: function() {
                var a = this.components.dataset;
                if (a && (a = a[0].components.data) && a[0])
                    return a = a[0].config, k(a.setValue, a.itemValue)
            },
            _createDatasets: function() {
                var a = this.components, b = this.jsonData, d = b.value, k = b.target, u = this.defaultDatasetType, e, C, b = [];
                a.dataset || (a.dataset = []);
                b.push({
                    value: d,
                    target: k
                });
                d = {
                    data: b
                };
                this.config.categories = b;
                b = a.dataset || (a.dataset = []);
                u && (C = E.get("component",
                ["dataset", u])) && (k = "datasetGroup_" + u, e = E.register("component", ["datasetGroup", u]), u = a[k], e&&!u && (u = a[k] = new e, u.chart = this, u.init()), b[0] ? (a = b[0].JSONData, a = a.data.length, u = d.data.length, a > u && b[0].removeData(u - 1, a - u, !1), b[0].JSONData = d, b[0].configure()) : (a = new C, b.push(a), a.chart = this, u && u.addDataSet(a, 0, 0), a.init(d)))
            },
            _createAxes: function() {
                var a = this.components, b = E.register("component", ["axis", "gauge"]);
                a.scale = a = new b;
                a.chart = this;
                a.init()
            }
        }, b.axisgaugebase)
    }
    ]);
    E.register("module", ["private",
    "modules.renderer.js-vbullet", function() {
        var b = this.hcLib, k = b.pluck, d = b.pluckNumber, a = b.chartAPI, n = b.pluckFontSize, L=!b.CREDIT_REGEX.test(this.window.location.hostname);
        a("vbullet", {
            friendlyName: "Vertical Bullet Gauge",
            creditLabel: L,
            defaultSeriesType: "bullet",
            gaugeType: 4,
            ticksOnRight: 0,
            standaloneInit: !0,
            hasCanvas: !0,
            singleseries: !0,
            isHorizontal: !1,
            isAxisOpposite: !1,
            isAxisReverse: !1,
            defaultDatasetType: "bullet",
            applicableDSList: {
                bullet: !0
            },
            defaultPaletteOptions: {
                paletteColors: [["A6A6A6", "CCCCCC", "E1E1E1",
                "F0F0F0"], ["A7AA95", "C4C6B7", "DEDFD7", "F2F2EE"], ["04C2E3", "66E7FD", "9CEFFE", "CEF8FF"], ["FA9101", "FEB654", "FED7A0", "FFEDD5"], ["FF2B60", "FF6C92", "FFB9CB", "FFE8EE"]],
                bgColor: ["FFFFFF", "CFD4BE,F3F5DD", "C5DADD,EDFBFE", "A86402,FDC16D", "FF7CA0,FFD1DD"],
                bgAngle: [270, 270, 270, 270, 270],
                bgRatio: ["0,100", "0,100", "0,100", "0,100", "0,100"],
                bgAlpha: ["100", "60,50", "40,20", "20,10", "30,30"],
                toolTipBgColor: ["FFFFFF", "FFFFFF", "FFFFFF", "FFFFFF", "FFFFFF"],
                toolTipBorderColor: ["545454", "545454", "415D6F", "845001", "68001B"],
                baseFontColor: ["333333", "60634E", "025B6A", "A15E01", "68001B"],
                tickColor: ["333333", "60634E", "025B6A", "A15E01", "68001B"],
                trendColor: ["545454", "60634E", "415D6F", "845001", "68001B"],
                plotFillColor: ["545454", "60634E", "415D6F", "845001", "68001B"],
                borderColor: ["767575", "545454", "415D6F", "845001", "68001B"],
                borderAlpha: [50, 50, 50, 50, 50]
            },
            _createAxes: function() {
                var a = this.components, b = E.register("component", ["axis", "gauge"]);
                a.scale = a = new b;
                a.chart = this;
                a.init()
            },
            _feedAxesRawData: function() {
                var a = this.components,
                u = a.colorManager, e = this.jsonData.chart, C = b.chartPaletteStr.chart2D, E = d(e.ticksbelowgraph, 1), E = d(e.ticksonright, e.axisontop, void 0 !== e.axisonleft?!d(e.axisonleft) : void 0, !E, this.isAxisOpposite), u = {
                    outCanfontFamily: k(e.outcnvbasefont, e.basefont, "Verdana,sans"),
                    outCanfontSize: n(e.outcnvbasefontsize, e.basefontsize, 10),
                    outCancolor: k(e.outcnvbasefontcolor, e.basefontcolor, u.getColor(C.baseFontColor)).replace(/^#?([a-f0-9]+)/ig, "#$1"),
                    useEllipsesWhenOverflow: e.useellipseswhenoverflow,
                    divLineColor: k(e.vdivlinecolor,
                    u.getColor(C.divLineColor)),
                    divLineAlpha: k(e.vdivlinealpha, u.getColor("divLineAlpha")),
                    divLineThickness: d(e.vdivlinethickness, 1),
                    divLineIsDashed: !!d(e.vdivlinedashed, e.vdivlineisdashed, 0),
                    divLineDashLen: d(e.vdivlinedashlen, 4),
                    divLineDashGap: d(e.vdivlinedashgap, 2),
                    showAlternateGridColor: d(e.showalternatevgridcolor, 0),
                    alternateGridColor: k(e.alternatevgridcolor, u.getColor("altVGridColor")),
                    alternateGridAlpha: k(e.alternatevgridalpha, u.getColor("altVGridAlpha")),
                    numDivLines: e.numvdivlines,
                    labelFont: e.labelfont,
                    labelFontSize: e.labelfontsize,
                    labelFontColor: e.labelfontcolor,
                    labelFontAlpha: e.labelalpha,
                    labelFontBold: e.labelfontbold,
                    labelFontItalic: e.labelfontitalic,
                    axisName: e.xaxisname,
                    axisMinValue: e.lowerlimit,
                    axisMaxValue: e.upperlimit,
                    setAdaptiveMin: e.setadaptivexmin,
                    adjustDiv: e.adjustvdiv,
                    labelDisplay: e.labeldisplay,
                    showLabels: e.showlabels,
                    rotateLabels: e.rotatelabels,
                    slantLabel: d(e.slantlabels, e.slantlabel),
                    labelStep: d(e.labelstep, e.xaxisvaluesstep),
                    showAxisValues: d(e.showxaxisvalues, e.showxaxisvalue),
                    showDivLineValues: d(e.showvdivlinevalues,
                    e.showvdivlinevalues),
                    showZeroPlane: e.showvzeroplane,
                    zeroPlaneColor: e.vzeroplanecolor,
                    zeroPlaneThickness: e.vzeroplanethickness,
                    zeroPlaneAlpha: e.vzeroplanealpha,
                    showZeroPlaneValue: e.showvzeroplanevalue,
                    trendlineColor: e.trendlinecolor,
                    trendlineToolText: e.trendlinetooltext,
                    trendlineThickness: e.trendlinethickness,
                    trendlineAlpha: e.trendlinealpha,
                    showTrendlinesOnTop: e.showtrendlinesontop,
                    showAxisLine: d(e.showxaxisline, e.showaxislines, e.drawAxisLines, 0),
                    axisLineThickness: d(e.xaxislinethickness, e.axislinethickness,
                    1),
                    axisLineAlpha: d(e.xaxislinealpha, e.axislinealpha, 100),
                    axisLineColor: k(e.xaxislinecolor, e.axislinecolor, "#000000"),
                    majorTMNumber: e.majortmnumber,
                    majorTMColor: e.majortmcolor,
                    majorTMAlpha: e.majortmalpha,
                    majorTMHeight: e.majortmheight,
                    tickValueStep: e.tickvaluestep,
                    showTickMarks: e.showtickmarks,
                    connectTickMarks: e.connecttickmarks,
                    showTickValues: e.showtickvalues,
                    majorTMThickness: e.majortmthickness,
                    upperlimit: a.numberFormatter.getCleanValue(e.upperlimit),
                    lowerlimit: a.numberFormatter.getCleanValue(e.lowerlimit),
                    reverseScale: e.reversescale,
                    showLimits: d(e.showlimits, e.showtickmarks),
                    adjustTM: e.adjusttm,
                    minorTMNumber: d(e.minortmnumber, 0),
                    minorTMColor: e.minortmcolor,
                    minorTMAlpha: e.minortmalpha,
                    minorTMHeight: d(e.minortmheight, e.minortmwidth),
                    minorTMThickness: e.minortmthickness,
                    tickMarkDistance: d(e.tickmarkdistance, e.tickmarkgap),
                    tickValueDistance: d(e.tickvaluedistance, e.displayvaluedistance),
                    placeTicksInside: e.placeticksinside,
                    placeValuesInside: e.placevaluesinside,
                    upperLimitDisplay: e.upperlimitdisplay,
                    lowerLimitDisplay: e.lowerlimitdisplay
                },
                a = a.scale;
                a.chart = this;
                a.setCommonConfigArr(u, !this.isHorizontal, !1, E);
                a.configure()
            },
            _drawCanvas: function() {}
        }, a.vled)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-hled", function() {
        var b = this.hcLib, k=!b.CREDIT_REGEX.test(this.window.location.hostname), b = b.chartAPI;
        b("hled", {
            friendlyName: "Vertical LED Gauge",
            defaultSeriesType: "led",
            defaultPlotShadow: 1,
            standaloneInit: !0,
            realtimeEnabled: !0,
            chartleftmargin: 15,
            chartrightmargin: 15,
            charttopmargin: 10,
            chartbottommargin: 10,
            showTooltip: 0,
            connectTickMarks: 0,
            isHorizontal: !0,
            isAxisOpposite: !1,
            creditLabel: k
        }, b.vled)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-hlineargauge", function() {
        var b = this.hcLib, k = b.pluck, d = b.getValidValue, a = b.BLANKSTRING, n = b.getDashStyle, L = b.getFirstValue, D = b.parseUnsafeString, u = b.preDefStr, e = u.animationObjStr, C = u.configStr, N = b.pluckNumber, J = b.getFirstDefinedValue, K = b.graphics.convertColor, I = b.getColorCodeString, G = b.COMMASTRING, Z = Math.max, S = b.toRaphaelColor, c = b.priorityList, F = b.schedular, g = u.POSITION_TOP, H = u.POSITION_BOTTOM,
        u=!b.CREDIT_REGEX.test(this.window.location.hostname), B = b.chartAPI;
        B("hlineargauge", {
            showRTvalue: !1,
            canvasPadding: !1,
            friendlyName: "Horizontal Linear Gauge",
            creditLabel: u,
            defaultDatasetType: "hlineargauge",
            standaloneInit: !0,
            isHorizontal: !0,
            isAxisOpposite: !1,
            hasLegend: !1,
            drawPlotlines: !1,
            drawPlotBands: !1,
            isAxisReverse: !1,
            minorTMNumber: 4,
            isRealTime: !0,
            colorRange: !0,
            applicableDSList: {
                hlineargauge: !0
            },
            rtParserModify: !0,
            _drawCanvas: function() {
                var a = this.components, b = this.config, c = this.graphics.datasetGroup,
                d = b.canvasWidth, p = b.canvasHeight, n = b.canvasTop, v = b.canvasLeft, l = a.scale, u = l.config.axisRange.min, F = l.config.axisRange.max, m = this.jsonData, l = m.chart, h = m.trendpoints && m.trendpoints.point, m = N(l.showgaugeborder, 1), w = J(l.colorrangefillmix, l.gaugefillmix, "{light-10},{dark-20},{light-50},{light-85}"), q = J(l.colorrangefillratio, l.gaugefillratio, l.gaugefillratio, "0,8,84,8"), B = k(l.colorrangebordercolor, l.gaugebordercolor, "{dark-20}"), A = N(l.colorrangeborderalpha, l.gaugeborderalpha, 100), x = m ? N(l.colorrangeborderthickness,
                l.gaugeborderthickness, 1): 0, D = a.colorRange && a.colorRange.getColorRangeArr(u, F), E = N(l.showshadow, 1), t, L, ma, R, Aa = a.paper, Q = a.colorManager, ya, ua, M, l = this.get(C, e), Ba = l.duration, za = l.dummyObj, fa = l.animObj, sa = l.animType, ha = {
                    top: 1,
                    bottom: 3
                }, da = l = 0, m = 0, a = a.canvas.graphics;
                b.gaugeStartX = b.canvasLeft;
                b.gaugeEndX = b.canvasLeft + d;
                b.gaugeStartY = b.canvasTop;
                b.gaugeEndY = b.canvasTop + p;
                b.gaugeCenterX = b.canvasLeft + d / 2;
                b.gaugeCenterY = b.canvasTop + p / 2;
                c.transform(["T", v, n]);
                (n = a.linear) || (a.linear = n = Aa.group("colorrange",
                c), n.trackTooltip(!0), a.outerRect = Aa.rect(c));
                a.outerRect.attr({
                    x: 0,
                    y: 0,
                    width: d,
                    height: p,
                    stroke: "none",
                    r: 0
                });
                t = function(a, b) {
                    return {
                        x: a * d / (F - u),
                        y: 0,
                        width: (b - a) * d / (F - u),
                        height: p
                    }
                };
                a.colorRangeElems || (a.colorRangeElems = []);
                c = 0;
                for (v = D && D.length; c < v; c += 1)
                    ma = D[c], R = t(ma.minvalue - u, ma.maxvalue - u), ma.x = R.x, ma.y = R.y, ma.width = R.width, ma.height = R.height, L = ma.code, L = K(I(k(ma.bordercolor, L), B), N(ma.borderalpha, A)), ya = Q.parseColorMix(ma.code, w), ua = Q.parseAlphaList(ma.alpha, ya.length), M = N(ma.borderAlpha, A), ma =
                    ua.split(G), ma = Z.apply(Math, ma), ma = Z(x && M || 0, ma), M = {
                        x: R.x,
                        y: R.y,
                        width: R.width,
                        height: R.height,
                        r: 0,
                        "stroke-width": x
                    }, (R = a.colorRangeElems[c]) || (R = a.colorRangeElems[c] = Aa.rect(n), R.attr(M)), R.attr({
                        stroke: L,
                        fill: S({
                            FCcolor: {
                                color: ya.toString(),
                                ratio: q,
                                alpha: ua,
                                angle: 270
                            }
                        })
                    }), R.animateWith(za, fa, M, Ba, sa), R.shadow({
                        apply: E,
                        opacity: ma / 100
                    }), R.show();
                for (; a.colorRangeElems[c];)
                    a.colorRangeElems[c].shadow(!1), a.colorRangeElems[c].hide(), c++;
                if (h)
                    for (b = b.trendPointConfig, a.trendObjElems || (a.trendObjElems =
                    []), a.trendZoneElems || (a.trendZoneElems = []), a.marker || (a.marker = []), c = 0, v = b.length; c < v; c += 1)
                        h = b[c], R = t(h.startValue - u, h.endValue - u), h.isTrendZone ? ((w = a.trendZoneElems[l]) || (w = a.trendZoneElems[l] = Aa.rect({
                            height: 0 < R.height ? R.height: 0
                        }, n)), w.attr({
                            fill: S({
                                FCcolor: {
                                    color: h.color,
                                    alpha: h.alpha
                                }
                            })
                        }), w.animateWith(za, fa, {
                            x: R.x,
                            y: R.y,
                            width: 0 < R.width ? R.width: 0,
                            height: 0 < R.height ? R.height: 0,
                            r: 0,
                            "stroke-width": 0
                        }, Ba, sa).tooltip(h.tooltext), w.show(), l++) : ((w = a.trendObjElems[da]) || (w = a.trendObjElems[da] = Aa.path(n)),
                        w.attr({
                            stroke: K(h.color, h.alpha),
                            "stroke-dasharray": h.dashStyle,
                            "stroke-width": h.thickness
                        }), w.animateWith(za, fa, {
                            path: ["M", R.x, R.y, "L", R.x, R.y + R.height]
                        }, Ba, sa).tooltip(h.tooltext), w.show(), da++), h.useMarker && (h.showOnTop ? (q = H, w = 0) : (q = g, w = p), B = 90 * ha[q], (q = a.marker[m]) || (a.marker[m] = q = Aa.polypath(n)), q.attr({
                            fill: h.markerColor,
                            stroke: h.markerBorderColor
                        }), q.animateWith(za, fa, {
                            polypath: [3, R.x, w, h.markerRadius, B, 0],
                            "stroke-width": 1
                        }, Ba, sa).shadow({
                            apply: E
                        }).tooltip(h.tooltext), q.show(), m++);
                if (E = a.trendObjElems)
                    for (; E[da];)
                        E[da].hide(),
                        da++;
                if (E = a.trendZoneElems)
                    for (; E[l];)
                        E[l].hide(), l++;
                if (q = a.marker)
                    for (; q[m];)
                        q[m].hide(), q[m].shadow(!1), m++
            },
            _configueTrendPoints: function() {
                var c = this.jsonData, g = this.config, e = g.style, u = c.trendpoints && c.trendpoints.point, p, H = this.components, v = H.scale.config, l = v.axisRange, F = l.max, B = l.min, v = v.scaleFactor || 1, m = H.colorManager, h, w, q, G = g.trendPointConfig = [], c = c.chart, A = u.length;
                e.trendStyle = {
                    fontFamily: e.outCanfontFamily,
                    color: e.outCancolor,
                    fontSize: e.outCanfontSize
                };
                for (p = 0; p < A; p++)
                    e = u[p], h = N(e.startvalue,
                    e.value), w = N(e.endvalue, h), q = h !== w, h <= F && h >= B && w <= F && w >= B && G.push({
                        startValue: h,
                        endValue: w,
                        tooltext: d(D(e.markertooltext)),
                        displayValue: d(D(e.displayvalue), q ? a : H.numberFormatter.scale(h)),
                        showOnTop: N(e.showontop, c.ticksbelowgauge, 1),
                        color: k(e.color, m.getColor("trendLightColor")),
                        textColor: e.color,
                        alpha: N(e.alpha, 99),
                        thickness: N(e.thickness, 1),
                        dashStyle: Number(e.dashed) ? n(e.dashlen || 2, e.dashgap || 2, e.thickness || 1): a,
                        useMarker: N(e.usemarker, 0),
                        markerColor: K(k(e.markercolor, e.color, m.getColor("trendLightColor")),
                        100),
                        markerBorderColor: K(k(e.markerbordercolor, e.bordercolor, m.getColor("trendDarkColor")), 100),
                        markerRadius: N(N(e.markerradius) * v, 5),
                        markerToolText: L(e.markertooltext),
                        trendValueDistance: N(N(e.trendvaluedistance) * v, l.tickInterval),
                        isTrendZone: q
                    });
                b.stableSort && b.stableSort(g.trendPointConfig, function(a, b) {
                    return a.startValue - b.startValue
                })
            },
            _createDatasets: function() {
                var a = this.components, b = this.jsonData.pointers, c;
                c = this.defaultDatasetType;
                var d, a = a.dataset || (a.dataset = []);
                c && (c = E.get("component",
                ["dataset", c])) && (a[0] ? (c = a[0].pointerArr && a[0].pointerArr.pointer && a[0].pointerArr.pointer.length, d = b && b.pointer && b.pointer.length || 0, c > d && a[0].removeData(c - d), a[0].pointerArr = b, a[0].configure()) : (c = new c, a.push(c), c.chart = this, c.init(b)))
            },
            _getData: function(a, b) {
                var d = this.components.dataset, g, e, k = this.getJobList(), n = function() {
                    return (e = d[0].components.data) && e[--a] ? (g = e[a].config, N(g.setValue, g.itemValue)) : null
                };
                if (d)
                    if ("function" === typeof b)
                        k.eiMethods.push(F.addJob(function() {
                            b(n())
                        }, c.postRender));
                    else 
                        return n()
            },
            _setData: function(a, b) {
                var c = "value=", d;
                if (void 0 !== a && void 0 !== b) {
                    for (d = 1; d < Number(a); d++)
                        c += G;
                    b.toString && (c += b.toString());
                    c && this.feedData(c)
                }
            },
            _getDataForId: function(a, b) {
                var d = this.components.dataset[0].idMap, g = this.getJobList();
                if ("function" === typeof b)
                    g.eiMethods.push(F.addJob(function() {
                        b(d && d[a] && d[a].config.itemValue || null)
                    }, c.postRender));
                else 
                    return d && d[a] && d[a].config.itemValue || null
            },
            _setDataForId: function(a, b) {
                var c = this.components.dataset[0].idMap;
                return c && c[a] &&
                this._setData(c[a].index + 1, b)
            }
        }, B.axisgaugebase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-hbullet", function() {
        var b = this.hcLib, k = b.pluck, d = b.pluckNumber, a = b.parseUnsafeString, n=!b.CREDIT_REGEX.test(this.window.location.hostname), E = b.chartAPI, D = b.BLANKSTRING, b = b.preDefStr, u = b.POSITION_START, e = b.POSITION_END, C = b.POSITION_BOTTOM, N = b.POSITION_MIDDLE, b = Math, J = b.ceil, K = b.max;
        E("hbullet", {
            friendlyName: "Horizontal Bullet Gauge",
            creditLabel: n,
            defaultSeriesType: "hbullet",
            gaugeType: 1,
            standaloneInit: !0,
            defaultCaptionPadding: 5,
            rendererId: "hbullet",
            isHorizontal: !0,
            isAxisOpposite: !0,
            rtManageSpace: !0,
            _RTmanageSpace: function() {
                var a = this.config, b = this.components, d = b.scale, b = b.dataset[0]._manageSpaceHorizontal(.7 * a.oriCanvasWidth);
                this._allocateSpace({
                    right: b.right - a.labelSpace.right
                });
                d.setAxisDimention({
                    axisLength: a.canvasWidth
                });
                a.labelSpace = b
            },
            _spaceManager: function() {
                var a, b = this.config, e = this.components, k = e.dataset[0], c = e.scale, n = this.jsonData.chart, g = d(n.showborder, this.is3D ? 0 : 1), u, B = b.minChartWidth,
                f = b.minChartHeight, n = b.borderWidth = g ? d(n.borderthickness, 1): 0;
                b.canvasWidth - 2 * n < B && (u = (b.canvasWidth - B) / 2);
                b.canvasHeight - 2 * n < f && (a = (b.canvasHeight - f) / 2);
                this._allocateSpace({
                    top: a || n,
                    bottom: a || n,
                    left: u || n,
                    right: u || n
                });
                a = .7 * b.canvasWidth;
                this._allocateSpace(c.placeAxis(b.availableHeight));
                this._allocateSpace(this._manageActionBarSpace && this._manageActionBarSpace(.225 * b.availableHeight) || {});
                b.oriCanvasWidth = b.canvasWidth;
                b.labelSpace = k._manageSpaceHorizontal(a);
                k._manageSpace && this._allocateSpace(b.labelSpace);
                this._manageChartMenuBar(.225 * b.canvasHeight);
                b.oriCanvasWidth -= K(e.subCaption.config.width || 0, e.caption.config.width || 0)
            },
            _manageCaptionSpacing: function() {
                var b = this.config, n = this.components, C = n.caption, E = n.subCaption, n = C.config, c = E.config, C = C.components, E = E.components, F = this.jsonData.chart, g = this.linkedItems.smartLabel, H = a(F.caption), B = a(F.subcaption), F = d(F.captionpadding, 2), f = b.height, r = 0, y = 0, K = 0, p = .7 * b.width, L = {}, v, l;
                3 < .7 * f && (n.captionPadding = c.captionPadding = F, H !== D && (l = n.style, r = n.captionLineHeight =
                J(parseFloat(k(l.fontHeight, l.lineHeight), 10), 12)), B !== D && (v = c.style, y = J(parseInt(k(v.lineHeight, v.fontHeight), 10), 12)), 0 < r || 0 < y) && (g.useEllipsesOnOverflow(b.useEllipsesWhenOverflow), g.setStyle(l), b = g.getSmartText(H, p, f), 0 < b.width && (b.width += 2, K = b.height), g.setStyle(v), v = g.getSmartText(B, p, f - K), 0 < v.width && (v.width += 2), n.captionSubCaptionGap = b.height + 0 + .2 * y, C.text = b.text, n.height = b.height, n.width = b.width, n.tooltext && (C.originalText = b.tooltext), E.text = v.text, c.height = v.height, c.width = v.width, c.tooltext &&
                (C.originalText = v.tooltext), v = Math.max(b.width, v.width), 0 < v && (v += F), n.maxCaptionWidth = c.maxCaptionWidth = v, n.isOnLeft ? L.left = v : L.right = v);
                n.align = n.isOnLeft ? c.align = e : c.align = u;
                return L
            },
            _manageCaptionPosition: function() {
                var a = this.config, b = this.components, d = b.caption.config, b = b.subCaption.config, e = d.captionPosition, c = K(d.width, b.width), k = a.borderWidth || 0, g = d.captionSubCaptionGap;
                switch (e) {
                case N:
                    d.y = .5 * (a.canvasTop + a.canvasHeight);
                    break;
                case C:
                    d.y = a.canvasBottom - (d.height + b.height);
                    break;
                default:
                    d.y =
                    a.canvasTop
                }
                b.y = d.y + g;
                d.x = d.isOnLeft ? b.x = a.marginLeft + c + k : b.x = a.width - a.marginRight - c - k
            },
            _fetchCaptionPos: function() {
                return this.components.caption.config.align === e ? 0 : - 1
            }
        }, E.vbullet)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-thermometer", function() {
        var b = this.hcLib, k = b.chartAPI, d = b.pluck, a = b.pluckNumber, n=!b.CREDIT_REGEX.test(this.window.location.hostname), L = b.preDefStr, D = b.HUNDREDSTRING, u = b.graphics.convertColor, e = L.gaugeFillColorStr, C = L.gaugeBorderColorStr, N = b.graphics.getLightColor,
        J = function(a) {
            return void 0 !== a && null !== a
        }, L = b.extend2;
        k("thermometer", {
            showRTvalue: !1,
            canvasPadding: !1,
            friendlyName: "Horizontal Linear Gauge",
            creditLabel: n,
            defaultDatasetType: "thermometer",
            defaultPaletteOptions: function(a, b) {
                var d;
                a || (a = {});
                for (d in b)
                    a[d] = b[d];
                return a
            }(L({}, b.defaultGaugePaletteOptions), {
                gaugeBorderColor: ["545454", "60634E", "415D6F", "845001", "68001B"],
                gaugeFillColor: ["999999", "ADB68F", "A2C4C8", "FDB548", "FF7CA0"],
                periodColor: ["EEEEEE", "ECEEE6", "E6ECF0", "FFF4E6", "FFF2F5"]
            }),
            standaloneInit: !0,
            isHorizontal: !1,
            isAxisOpposite: !0,
            hasLegend: !1,
            hasCanvas: !1,
            drawPlotlines: !1,
            drawPlotBands: !1,
            isAxisReverse: !1,
            isRealTime: !0,
            applicableDSList: {
                thermometer: !0
            },
            _getData: function() {
                var a = this.components.dataset;
                if (a && a[0])
                    return a[0].config.value
            },
            _parseSpecialConfig: function() {
                var b = this.config, k = this.jsonData.chart, n = this.components, E = n.numberFormatter, n = n.colorManager;
                b.use3DLighting = a(k.use3dlighting, 1);
                b.thmOriginX = a(k.thmoriginx, k.gaugeoriginx);
                b.thmOriginY = a(k.thmoriginy, k.gaugeoriginy);
                b.thmBulbRadius =
                a(E.getCleanValue(k.thmbulbradius, !0));
                b.thmHeight = a(E.getCleanValue(a(k.thmheight, k.gaugeheight), !0));
                b.origW = a(k.origw);
                b.origH = a(k.origh);
                b.xDefined = J(b.thmOriginX);
                b.yDefined = J(b.thmOriginY);
                b.rDefined = J(b.thmBulbRadius);
                b.hDefined = J(b.thmHeight);
                b.gaugeFillColor = d(k.gaugefillcolor, k.thmfillcolor, n.getColor(e));
                b.gaugeFillAlpha = a(k.gaugefillalpha, k.thmfillalpha, D);
                b.showGaugeBorder = a(k.showgaugeborder, 1);
                E = b.showGaugeBorder ? a(k.gaugeborderalpha, 40) : 0;
                b.gaugeBorderColor = u(d(k.gaugebordercolor,
                n.getColor(C)), E);
                b.gaugeBorderThickness = a(k.gaugeborderthickness, 1);
                b.gaugeContainerColor = d(k.thmglasscolor, N(b.gaugeFillColor, 30))
            },
            _createDatasets: function() {
                var a = this.components, b;
                b = this.defaultDatasetType;
                var d = {
                    data: [{
                        value: this.jsonData.value
                    }
                    ]
                }, a = a.dataset || (a.dataset = []);
                b && (b = E.get("component", ["dataset", b])) && (a[0] ? (a[0].setValue(d && d.data && d.data[0]), a[0].configure()) : (b = new b, a.push(b), b.chart = this, b.init(d)))
            }
        }, k.axisgaugebase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-cylinder",
    function() {
        var b = this.hcLib, k = b.HUNDREDSTRING, d = b.pluck, a = b.pluckNumber, n = b.extend2, E = b.graphics.getLightColor, D = b.graphics.convertColor, u = b.chartAPI, e = b.preDefStr, C = e.colors.FFFFFF, N = e.gaugeFillColorStr, J = e.gaugeBorderColorStr, K = function(a) {
            return void 0 !== a && null !== a
        };
        u("cylinder", {
            defaultDatasetType: "cylinder",
            applicableDSList: {
                cylinder: !0
            },
            defaultPaletteOptions: function(a, b) {
                var d;
                a || (a = {});
                for (d in b)
                    a[d] = b[d];
                return a
            }(n({}, b.defaultGaugePaletteOptions), {
                gaugeBorderColor: ["545454", "60634E",
                "415D6F", "845001", "68001B"],
                gaugeFillColor: ["CCCCCC", "ADB68F", "E1F5FF", "FDB548", "FF7CA0"],
                periodColor: ["EEEEEE", "ECEEE6", "E6ECF0", "FFF4E6", "FFF2F5"]
            }),
            glasscolor: C,
            _parseSpecialConfig: function() {
                var b = this.config, e = this.jsonData.chart, n = this.components, u = n.numberFormatter, n = n.colorManager;
                b.use3DLighting = a(e.use3dlighting, 1);
                b.gaugeOriginX = a(e.thmoriginx, e.cyloriginx, e.gaugeoriginx);
                b.gaugeOriginY = a(e.thmoriginy, e.cyloriginy, e.gaugeoriginy);
                b.gaugeRadius = a(u.getCleanValue(a(e.thmbulbradius, e.cylradius,
                e.gaugeradius), !0));
                b.gaugeHeight = a(u.getCleanValue(a(e.thmheight, e.cylheight, e.gaugeheight), !0));
                b.origW = a(e.origw);
                b.origH = a(e.origh);
                b.xDefined = K(b.gaugeOriginX);
                b.yDefined = K(b.gaugeOriginY);
                b.rDefined = K(b.gaugeRadius);
                b.hDefined = K(b.gaugeHeight);
                b.gaugeFillColor = d(e.gaugefillcolor, e.cylfillcolor, n.getColor(N));
                b.gaugeFillAlpha = a(e.gaugefillalpha, e.cylfillalpha, k);
                b.gaugeYScale = a(e.cylyscale, e.gaugeyscale, 30);
                if (50 < b.gaugeYScale || 0 > b.gaugeYScale)
                    b.gaugeYScale = 30;
                b.gaugeYScale/=100;
                b.showGaugeBorder =
                a(e.showgaugeborder, 1);
                u = b.showGaugeBorder ? a(e.gaugeborderalpha, 40) : 0;
                b.gaugeBorderColor = D(d(e.gaugebordercolor, n.getColor(J)), u);
                b.gaugeBorderThickness = a(e.gaugeborderthickness, 1);
                b.gaugeContainerColor = d(e.cylglasscolor, E(b.gaugeFillColor, 30))
            }
        }, u.thermometer)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-angulargauge", function() {
        var b = this.hcLib, k = b.pluck, d = b.getValidValue, a = b.BLANKSTRING, n = b.preDefStr, L = n.animationObjStr, D = n.configStr, u = b.pluckNumber, e = b.graphics.convertColor, C = b.COMMASTRING,
        n = Math, N = n.max, J = n.min, K = n.PI / 180, I = b.toRaphaelColor, n=!b.CREDIT_REGEX.test(this.window.location.hostname), G = b.chartAPI, Z = b.extend2, S = b.pluckFontSize;
        G("angulargauge", {
            friendlyName: "Angular Gauge",
            creditLabel: n,
            defaultDatasetType: "angulargauge",
            standaloneInit: !0,
            isHorizontal: !0,
            isAxisOpposite: !1,
            isRealTime: !0,
            hasLegend: !1,
            drawPlotlines: !1,
            drawPlotBands: !1,
            isAxisReverse: !1,
            colorRange: !0,
            defaultPaletteOptions: function(a, b) {
                var d;
                a || (a = {});
                for (d in b)
                    a[d] = b[d];
                return a
            }(Z({}, b.defaultGaugePaletteOptions),
            {
                dialColor: ["999999,ffffff,999999", "ADB68F,F3F5DD,ADB68F", "A2C4C8,EDFBFE,A2C4C8", "FDB548,FFF5E8,FDB548", "FF7CA0,FFD1DD,FF7CA0"],
                dialBorderColor: ["999999", "ADB68F", "A2C4C8", "FDB548", "FF7CA0"],
                pivotColor: ["999999,ffffff,999999", "ADB68F,F3F5DD,ADB68F", "A2C4C8,EDFBFE,A2C4C8", "FDB548,FFF5E8,FDB548", "FF7CA0,FFD1DD,FF7CA0"],
                pivotBorderColor: ["999999", "ADB68F", "A2C4C8", "FDB548", "FF7CA0"]
            }),
            rtParserModify: !0,
            applicableDSList: {
                angulargauge: !0
            },
            _spaceManager: function() {
                var b, e, g = this.config, k = this.components,
                n = k.scale.config, f = k.dataset[0];
                b = f.components.data[0];
                var k = k.scale, r = f.chart.jsonData.chart, f = f.config, y = f.scaleFactor, D = 0, p = 0, E = D = 0, v = 0, v = f.pivotRadius, l = g.dataLabels.style.fontSize, D = g.dataLabels.style.lineHeight, E = g.displayValueCount, C = g.borderWidth, G = g.minChartWidth, m = g.minChartHeight, v = 0, h;
                g.canvasWidth - 2 * C < G && (e = (g.canvasWidth - G) / 2);
                g.canvasHeight - 2 * C < m && (h = (g.canvasHeight - m) / 2);
                this._allocateSpace({
                    top: h || C,
                    bottom: h || C,
                    left: e || C,
                    right: e || C
                });
                g.scaleFactor = g.autoScale ? y = this._getScaleFactor(f.origW,
                f.origH, g.width, g.height) : y = 1;
                l.replace(/px/i, a);
                D = D.replace(/px/i, a);
                h = /^\d+\%$/.test(f.gaugeinnerradius) ? parseInt(f.gaugeinnerradius, 10) / 100 : .7;
                e = v = u(d(r.pivotradius) * y, 5);
                f.pivotRadius = e;
                b.rearextension && (v = Math.max(v, b.rearextension * y));
                f.compositPivotRadius = v;
                D = E * D + 2 + e;
                f.valueBelowPivot || (p = D, D = 0);
                f.gaugeOuterRadius = u(Math.abs(d(r.gaugeouterradius) * y));
                f.gaugeInnerRadius = u(Math.abs(d(r.gaugeinnerradius) * y), f.gaugeOuterRadius * h);
                b = .7 * g.canvasWidth;
                e = .7 * g.canvasHeight;
                b = k.placeAxis(J(b, e));
                e = .7 *
                g.canvasHeight;
                this._manageChartMenuBar(e);
                n = this._angularGaugeSpaceManager(f.gaugeStartAngle, f.gaugeEndAngle, g.canvasWidth, g.canvasHeight, f.gaugeOuterRadius, u(d(r.gaugeoriginx) * y - g.canvasLeft), u(d(r.gaugeoriginy) * y - g.canvasTop), v + n.polarPadding, D, p);
                D = n.radius = u(n.radius, n.maxRadius);
                f.gaugeOriginX = n.centerX + g.canvasLeft;
                f.gaugeOriginY = n.centerY + g.canvasTop;
                E = n.centerX;
                v = n.centerY;
                v = b.left < b.top ? E - b.left >= D - b.left && v - b.top >= D - b.left ? b.left : b.top : E - b.left >= D - b.top && v - b.top >= D - b.top ? b.top : b.left;
                v +=
                2 * k.config.polarPadding;
                f.gaugeOuterRadius || (f.gaugeOuterRadius = n.radius, f.gaugeOuterRadius -= v);
                void 0 === f.gaugeInnerRadius && (f.gaugeInnerRadius = f.gaugeOuterRadius * h);
                k.setAxisConfig({
                    centerX: f.gaugeOriginX,
                    centerY: f.gaugeOriginY,
                    radius: n.radius || f.gaugeOuterRadius,
                    gaugeOuterRadius: f.gaugeOuterRadius,
                    gaugeInnerRadius: f.gaugeInnerRadius,
                    scaleFactor: y
                });
                n = k.getLimit();
                k.getPixel(n.min);
                k.getPixel(n.max);
                g.gaugeStartX = g.canvasLeft;
                g.gaugeStartY = g.canvasTop;
                g.gaugeEndX = g.canvasRight;
                g.gaugeEndY = g.canvasBottom;
                g.gaugeCenterX = f.gaugeOriginX;
                g.gaugeCenterY = f.gaugeOriginY;
                g.gaugeStartAngle = f.gaugeStartAngle / K;
                g.gaugeEndAngle = f.gaugeEndAngle / K
            },
            _createAxes: function() {
                var a = this.components, b = E.register("component", ["axis", "polarGauge"]);
                a.scale = a = new b;
                a.chart = this;
                a.init()
            },
            _feedAxesRawData: function() {
                var a = this.components, d = a.colorManager, g = this.jsonData, e = g.chart, n = b.chartPaletteStr.chart2D, f = u(e.axisontop, e.axisonleft, void 0 !== e.ticksbelowgauge?!e.ticksbelowgauge : void 0, this.isAxisOpposite), r = u(e.reverseaxis,
                this.isAxisReverse), d = {
                    outCanfontFamily: k(e.outcnvbasefont, e.basefont, "Verdana,sans"),
                    outCanfontSize: S(e.outcnvbasefontsize, e.basefontsize, 10),
                    outCancolor: k(e.outcnvbasefontcolor, e.basefontcolor, d.getColor(n.baseFontColor)).replace(/^#?([a-f0-9]+)/ig, "#$1"),
                    useEllipsesWhenOverflow: e.useellipseswhenoverflow,
                    divLineColor: k(e.vdivlinecolor, d.getColor(n.divLineColor)),
                    divLineAlpha: k(e.vdivlinealpha, d.getColor("divLineAlpha")),
                    divLineThickness: u(e.vdivlinethickness, 1),
                    divLineIsDashed: !!u(e.vdivlinedashed,
                    e.vdivlineisdashed, 0),
                    divLineDashLen: u(e.vdivlinedashlen, 4),
                    divLineDashGap: u(e.vdivlinedashgap, 2),
                    showAlternateGridColor: u(e.showalternatevgridcolor, 0),
                    alternateGridColor: k(e.alternatevgridcolor, d.getColor("altVGridColor")),
                    alternateGridAlpha: k(e.alternatevgridalpha, d.getColor("altVGridAlpha")),
                    numDivLines: e.numvdivlines,
                    labelFont: e.labelfont,
                    labelFontSize: e.labelfontsize,
                    labelFontColor: e.labelfontcolor,
                    labelFontAlpha: e.labelalpha,
                    labelFontBold: e.labelfontbold,
                    labelFontItalic: e.labelfontitalic,
                    axisName: e.xaxisname,
                    axisMinValue: e.lowerlimit,
                    axisMaxValue: e.upperlimit,
                    setAdaptiveMin: e.setadaptivemin,
                    adjustDiv: e.adjustvdiv,
                    labelDisplay: e.labeldisplay,
                    showLabels: e.showlabels,
                    rotateLabels: e.rotatelabels,
                    slantLabel: u(e.slantlabels, e.slantlabel),
                    labelStep: u(e.labelstep, e.xaxisvaluesstep),
                    showAxisValues: u(e.showxaxisvalues, e.showxaxisvalue),
                    showDivLineValues: u(e.showvdivlinevalues, e.showvdivlinevalues),
                    showZeroPlane: e.showvzeroplane,
                    zeroPlaneColor: e.vzeroplanecolor,
                    zeroPlaneThickness: e.vzeroplanethickness,
                    zeroPlaneAlpha: e.vzeroplanealpha,
                    showZeroPlaneValue: e.showvzeroplanevalue,
                    trendlineColor: e.trendlinecolor,
                    trendlineToolText: e.trendlinetooltext,
                    trendlineThickness: e.trendlinethickness,
                    trendlineAlpha: e.trendlinealpha,
                    showTrendlinesOnTop: e.showtrendlinesontop,
                    showAxisLine: u(e.showxaxisline, e.showaxislines, e.drawAxisLines, 0),
                    axisLineThickness: u(e.xaxislinethickness, e.axislinethickness, 1),
                    axisLineAlpha: u(e.xaxislinealpha, e.axislinealpha, 100),
                    axisLineColor: k(e.xaxislinecolor, e.axislinecolor, "#000000"),
                    majorTMNumber: e.majortmnumber,
                    majorTMColor: e.majortmcolor,
                    majorTMAlpha: e.majortmalpha,
                    majorTMHeight: e.majortmheight,
                    tickValueStep: e.tickvaluestep,
                    showTickMarks: e.showtickmarks,
                    connectTickMarks: e.connecttickmarks,
                    showTickValues: e.showtickvalues,
                    majorTMThickness: e.majortmthickness,
                    upperlimit: a.numberFormatter.getCleanValue(e.upperlimit),
                    lowerlimit: a.numberFormatter.getCleanValue(e.lowerlimit),
                    reverseScale: e.reversescale,
                    showLimits: e.showlimits,
                    adjustTM: e.adjusttm,
                    minorTMNumber: e.minortmnumber,
                    minorTMColor: e.minortmcolor,
                    minorTMAlpha: e.minortmalpha,
                    minorTMHeight: u(e.minortmheight,
                    e.minortmwidth),
                    minorTMThickness: e.minortmthickness,
                    tickMarkDistance: u(e.tickmarkdistance, e.tickmarkgap),
                    tickValueDistance: u(e.tickvaluedistance, e.displayvaluedistance),
                    placeTicksInside: e.placeticksinside,
                    placeValuesInside: e.placevaluesinside,
                    upperLimitDisplay: e.upperlimitdisplay,
                    lowerLimitDisplay: e.lowerlimitdisplay,
                    ticksBelowGauge: e.ticksbelowgauge,
                    ticksBelowGraph: e.ticksbelowgraph,
                    trendValueDistance: e.trendvaluedistance
                };
                d.trendPoints = g.trendpoints;
                a = a.scale;
                a.setCommonConfigArr(d, !this.isHorizontal,
                r, f);
                a.configure()
            },
            _drawCanvas: function() {
                var a = this.components, b = a.dataset[0], d = b.config, b = b.graphics || (b.graphics = {}), n = a.scale, B = a.colorManager, f = n.config.axisRange, r = a.colorRange, y = this.graphics.datasetGroup, E = this.graphics.datalabelsGroup, a = a.paper, p = d.gaugeOuterRadius, G = d.gaugeInnerRadius, v = d.gaugeFillRatio, l = d.gaugeBorderColor, J = d.gaugeBorderThickness, K = d.gaugeBorderAlpha, m = d.gaugeFillMix, h = d.gaugeOriginX, w = d.gaugeOriginY, q = d.gaugeStartAngle, z = d.showShadow, A = f.min, f = f.max, x = r ? r.getColorRangeArr(A,
                f): [], S = this.get(D, L), r = S.duration, A = S.dummyObj, Z = S.animObj, S = S.animType, t = 0, pa = x.length, ma, R, Aa, Q, ya, ua, M = 0;
                b.band = b.band || [];
                b.bandGroup || (b.bandGroup = a.group("bandGroup", y));
                for (b.pointGroup ? b.pointGroup.animateWith(A, Z, {
                    transform: "t" + h + C + w
                }, r, S) : b.pointGroup = a.group("pointers", E).translate(h, w); t < pa; t += 1)
                    ma = x[t], y = n.getAngle(Math.min(ma.maxvalue, f)), Aa = B.parseColorMix(ma.code, m), Q = B.parseAlphaList(ma.alpha, Aa.length), ya = B.parseRatioList(G / p * 100 + v, Aa.length), ua = ma.bordercolor, R = u(ma.borderAlpha,
                    K), ua = ua&&-1 == ua.indexOf("{") ? e(ua, R) : B.parseColorMix(ma.code, k(ua, l))[0], ua = e(ua, R), ma = Q.split(C), ma = N.apply(Math, ma), ma = z ? N(J && R || 0, ma) : 0, R = y, q > y && (q += y, y = q - y, q -= y), b.band[t] ? b.band[t].animateWith(A, Z, {
                        ringpath: [h, w, p, G, q, y]
                    }, r, S) : b.band[t] = a.ringpath(h, w, p, G, q, y, b.bandGroup), b.band[t].attr({
                        fill: I({
                            FCcolor: {
                                cx: h,
                                cy: w,
                                r: p,
                                gradientUnits: "userSpaceOnUse",
                                color: Aa.join(),
                                alpha: Q,
                                ratio: ya,
                                radialGradient: !0
                            }
                        }),
                        "stroke-width": J,
                        stroke: ua
                    }).shadow({
                        apply: z,
                        opacity: ma / 100
                    }), q = R, M += 1;
                t = M;
                for (pa = b.band.length; t <
                pa; t += 1)
                    b.band[t].attr({
                        ringpath: [0, 0, 0, 0, 0]
                    });
                n = d.isRadialGradient ? {
                    color: d.pivotFillColor,
                    alpha: d.pivotFillAlpha,
                    ratio: d.pivotFillRatio,
                    radialGradient: !0,
                    angle: d.pivotFillAngle,
                    cx: .5,
                    cy: .5,
                    r: "50%"
                } : {
                    color: d.pivotFillColor,
                    alpha: d.pivotFillAlpha,
                    ratio: d.pivotFillRatio,
                    radialGradient: !1,
                    angle: d.pivotFillAngle
                };
                b.pivot ? b.pivot.animateWith(A, Z, {
                    cx: h,
                    cy: w,
                    r: d.pivotRadius
                }, r, S) : (b.pivot = a.circle(E), b.pivot.attr({
                    cx: h,
                    cy: w,
                    r: d.pivotRadius
                }));
                b.pivot.attr({
                    fill: I({
                        FCcolor: n
                    }),
                    "stroke-width": d.pivotBorderThickness,
                    stroke: d.pivotBorderColor
                }).shadow({
                    apply: z
                })
            },
            _createDatasets: function() {
                var a = this.components, b = this.jsonData, d = b.pointers || b.dials, e;
                e = this.defaultDatasetType;
                d || (b.dials = d = {
                    dial: [{
                        value: 0
                    }
                    ]
                });
                a = a.dataset || (a.dataset = []);
                e && (e = E.get("component", ["dataset", e])) && (a[0] ? (e = a[0].components.data && a[0].components.data.length, d = d.dial && d.dial.length || 0, e > d && a[0].removeData(e - d), a[0].configure()) : (e = new e, a.push(e), e.chart = this, e.init(d)))
            },
            _setCategories: function() {},
            _angularGaugeSpaceManager: function(a,
            b, d, e, k, f, n, u, D, p) {
                var E = void 0 !== k && null !== k, v = void 0 !== f && null !== f, l = void 0 !== n && null !== n, C = 2 * Math.PI, G = Math.PI, m = Math.PI / 2, h = G + m, w;
                k = {
                    radius: k,
                    centerX: f,
                    centerY: n
                };
                var q, z, A, x, L=!1, I, t = a%C;
                0 > t && (t += C);
                (u = u || 0) && u < d / 2 && u < e / 2 && (L=!0);
                D > e / 2 && (D = e / 2);
                p > e / 2 && (p = e / 2);
                z = Math.cos(a);
                x = Math.sin(a);
                A = Math.cos(b);
                I = Math.sin(b);
                q = Math.min(z, A, 0);
                A = Math.max(z, A, 0);
                z = Math.min(x, I, 0);
                x = Math.max(x, I, 0);
                if (!E ||!v ||!l) {
                    b -= a;
                    a = t + b;
                    if (a > C || 0 > a)
                        A = 1;
                    if (0 < b) {
                        if (t < m && a > m || a > C + m)
                            x = 1;
                        if (t < G && a > G || a > C + G)
                            q =- 1;
                        if (t < h && a > h ||
                        a > C + h)
                            z =- 1
                    } else {
                        if (t > m && a < m || a<-h)
                            x = 1;
                        if (t > G && a < G || a<-G)
                            q =- 1;
                        if (t > h && a < h || a<-m)
                            z =- 1
                    }
                    v ? E || (C = d - f, w = q ? Math.min(C / A, - f / q) : C / A) : (v = d / (A - q), f =- v * q, w = v, L && (d - f < u ? (f = d - u, C = d - f, w = q ? Math.min(C / A, - f / q) : C / A) : f < u && (f = u, C = d - f, w = q ? Math.min(C / A, - f / q) : C / A)), k.centerX = f);
                    l ? E || (C = e - n, w = Math.min(w, z ? Math.min(C / x, - n / z) : C / x)) : (v = e / (x - z), n =- v * z, L && (e - n < u ? (n = e - u, C = e - n, w = Math.min(w, z ? Math.min(C / x, - n / z) : C / x)) : n < u && (n = u, C = e - n, w = Math.min(w, z ? Math.min(C / x, - n / z) : C / x))), e - n < D ? (n = e - D, C = e - n, w = Math.min(w, z ? Math.min(C / x, - n / z) : C / x)) :
                    n < p && (n = p, C = e - n, w = Math.min(w, z ? Math.min(C / x, - n / z) : C / x)), w = Math.min(w, v), k.centerY = n);
                    k.maxRadius = w;
                    0 >= k.maxRadius && (k.maxRadius = Math.min(d / 2, e / 2))
                }
                return k
            },
            _getScaleFactor: function(a, b, d, e) {
                b = u(b, e);
                a = u(a, d);
                return b && a ? a / d == b / e ? d / a : Math.min(d / a, e / b) : 1
            },
            _setData: G.hlineargauge,
            _getData: G.hlineargauge,
            _getDataForId: G.hlineargauge,
            _setDataForId: G.hlineargauge
        }, G.axisgaugebase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-bulb", function() {
        var b = this.hcLib, k=!b.CREDIT_REGEX.test(this.window.location.hostname),
        d = b.chartAPI, a = b.pluckNumber;
        d("bulb", {
            showRTvalue: !1,
            canvasPadding: !1,
            friendlyName: "Bulb Gauge",
            defaultSeriesType: "bulb",
            defaultPlotShadow: 1,
            standaloneInit: !0,
            drawAnnotations: !0,
            charttopmargin: 10,
            chartrightmargin: 10,
            chartbottommargin: 10,
            chartleftmargin: 10,
            realtimeEnabled: !0,
            isRealTime: !0,
            defaultDatasetType: "bulb",
            applicableDSList: {
                bulb: !0
            },
            creditLabel: k,
            _createDatasets: function() {
                var a = this.components, b;
                b = this.defaultDatasetType;
                var d = [], k;
                d.push({
                    value: this.jsonData.value
                });
                k = {
                    data: d
                };
                this.config.categories =
                d;
                a = a.dataset || (a.dataset = []);
                b && (b = E.get("component", ["dataset", b])) && (a[0] ? (b = a[0].JSONData, b = b.data.length, d = k.data.length, b > d && a[0].removeData(d - 1, b - d, !1), a[0].JSONData = k, a[0].configure()) : (b = new b, a.push(b), b.chart = this, b.init(k)))
            },
            _drawCanvas: function() {},
            _spaceManager: function() {
                var b, d = this.hasLegend;
                b = this.config;
                var k = this.components, u = k.legend, k = k.dataset[0], e = k.config, C = this.jsonData.chart, E = a(C.showborder, this.is3D ? 0 : 1), J, K, I = b.minChartWidth, G = b.minChartHeight, C = b.borderWidth = E ? a(C.borderthickness,
                1): 0;
                e.scaleFactor = b.autoscale ? this._getScaleFactor(e.origW, e.origH, b.width, b.height) : 1;
                b.canvasWidth - 2 * C < I && (K = (b.canvasWidth - I) / 2);
                b.canvasHeight - 2 * C < G && (J = (b.canvasHeight - G) / 2);
                this._allocateSpace({
                    top: J || C,
                    bottom: J || C,
                    left: K || C,
                    right: K || C
                });
                this._allocateSpace(this._manageActionBarSpace && this._manageActionBarSpace(.225 * b.availableHeight) || {});
                b = .7 * b.canvasHeight;
                !1 !== d && this._allocateSpace(u._manageLegendPosition(b));
                this._manageChartMenuBar(b);
                k._manageSpace && this._allocateSpace(k._manageSpace(b))
            },
            _getData: d.vled,
            _getScaleFactor: d.angulargauge
        }, d.gaugebase, {
            placevaluesinside: 0,
            hasgaugeoriginx: void 0,
            gaugeoriginx: void 0,
            hasgaugeoriginy: void 0,
            gaugeoriginy: void 0,
            hasgaugeradius: void 0,
            gaugeradius: void 0,
            valuepadding: 2,
            showgaugeborder: 0,
            showhovereffect: void 0,
            autoscale: 1
        })
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-progressgauge", function() {
        var b = this.hcLib, k = b.chartAPI, b=!b.CREDIT_REGEX.test(this.window.location.hostname);
        k("progressgauge", {
            friendlyName: "Progress Gauge",
            creditLabel: b,
            defaultSeriesType: "progressgauge",
            singleseries: !0,
            gaugeType: 1,
            standaloneInit: !0,
            defaultCaptionPadding: 5,
            hasLegend: !0,
            defaultDatasetType: "progressgauge",
            applicableDSList: {
                progressgauge: !0
            },
            _createDatasets: function() {
                var b = this.components, a = b.legend, k = this.jsonData, L = k.dataset, D = k.data || L && L[0].data, u = this.defaultDatasetType, e, C, L = this._dataSegregator(D);
                this.config.categories = L.data;
                k = b.dataset || (b.dataset = []);
                if (!D || 0 === D.length)
                    this.setChartMessage();
                else if (u && (C = E.get("component", ["dataset", u])))
                    if (D =
                    "datasetGroup_" + u, e = E.register("component", ["datasetGroup", u]), u = b[D], e&&!u && (u = b[D] = new e, u.chart = this, u.init()), k[0]) {
                        u = k[0].JSONData;
                        b = k[0].components.data || [];
                        D = u.data.length;
                        C = L.data && L.data.length || 0;
                        if (D > C) {
                            if (a)
                                for (u = C; u < D; u++)
                                    b[u] && b[u].legendItemId && a.removeItem(b[u].legendItemId);
                                    k[0].removeData(C, D - C, !1)
                                }
                                k[0].JSONData = L;
                                k[0].configure()
                    } else 
                        a = new C, k.push(a), a.chart = this, u && u.addDataSet(a, 0, 0), a.index = 0, a.init(L)
                },
            getDataSet: function(b) {
                return this.components.dataset[b]
            }
        }, k.bulb)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-drawingpad", function() {
        var b = this.hcLib, k = b.Raphael, d = b.preDefStr, a = d.animationObjStr, n = d.configStr, E = d.ROUND, d = d.colors.FFFFFF, D=!b.CREDIT_REGEX.test(this.window.location.hostname), b = b.chartAPI;
        b("drawingpad", {
            standaloneInit: !0,
            friendlyName: "Drawing Pad",
            creditLabel: D,
            bgColor: d,
            bgAlpha: "100",
            draw: function() {
                var b = this.config, d = this.linkedItems.container, C = this.chartInstance, D = this.components, J = D.paper, K = this.get(n, a), I = K.animType, G = K.dummyObj, Z =
                K.animObj, K = K.duration, S = D.tooltip, c = b.prevWidth, F = b.prevHeight, g;
                b.width = d.offsetWidth;
                b.height = d.offsetHeight;
                this._show();
                b.animationStarted=!0;
                J ? ((c || F) && J.setSize(c, F), D = {
                    width: d.offsetWidth,
                    height: d.offsetHeight
                }, g=!0, this._chartAnimation(!0), J.animateWith(G, Z, D, K, I)) : (J = D.paper = new k(d, d.offsetWidth, d.offsetHeight), J.setConfig("stroke-linecap", E));
                b.prevWidth = d.offsetWidth;
                b.prevHeight = d.offsetHeight;
                J.tooltip(S.style, S.config.shadow, S.config.constrain);
                this._createLayers();
                !g && this._chartAnimation(!0);
                this._drawBackground();
                C.annotations ? (this._drawAnnotations(), this._drawCreditLabel()) : this.setChartMessage()
            },
            _createDatasets: function() {}
        }, b.mscartesian)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-realtimearea", function() {
        var b = this.hcLib.chartAPI;
        b("realtimearea", {
            defaultDatasetType: "realtimearea",
            axisPaddingLeft: 0,
            axisPaddingRight: 0,
            applicableDSList: {
                realtimearea: !0
            }
        }, b.realtimecolumn, {
            enablemousetracking: !0
        }, b.areabase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-realtimestackedarea",
    function() {
        var b = this.hcLib.chartAPI;
        b("realtimestackedarea", {
            defaultDatasetType: "realtimearea",
            applicableDSList: {
                realtimearea: !0
            }
        }, b.realtimearea, {
            isstacked: !0,
            enablemousetracking: !0
        })
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-realtimeline", function() {
        var b = this.hcLib.chartAPI;
        b("realtimeline", {
            defaultDatasetType: "realtimeline",
            axisPaddingLeft: 0,
            axisPaddingRight: 0,
            applicableDSList: {
                realtimeline: !0
            },
            zeroplanethickness: 1,
            zeroplanealpha: 40,
            showzeroplaneontop: 0
        }, b.realtimecolumn, {
            zeroplanethickness: 1,
            zeroplanealpha: 40,
            showzeroplaneontop: 0,
            enablemousetracking: !0
        }, b.areabase)
    }
    ]);
    E.register("module", ["private", "modules.renderer.js-realtimelinedy", function() {
        var b = this.hcLib.chartAPI;
        b("realtimelinedy", {
            isRealTime: !0,
            defaultDatasetType: "realtimeline",
            axisPaddingLeft: 0,
            isDual: !0,
            axisPaddingRight: 0,
            applicableDSList: {
                realtimeline: !0
            },
            _createAxes: b.msdybasecartesian._createAxes,
            _setAxisLimits: b.msdybasecartesian._setAxisLimits,
            _postSpaceManagement: b.msdybasecartesian._postSpaceManagement,
            _feedAxesRawData: b.msdybasecartesian._feedAxesRawData
        },
        b.realtimecolumn, {
            isdual: !0,
            zeroplanethickness: 1,
            zeroplanealpha: 40,
            showzeroplaneontop: 0,
            enablemousetracking: !0
        }, b.areabase)
    }
    ])
});


