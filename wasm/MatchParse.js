
class MatchResult {
    constructor(arrRes, originalQuery, originalTarget) {
        this.originalQuery = originalQuery;
        this.originalTarget = originalTarget;
        this.query = Module.UTF8ToString(arrRes[0]);
        this.align = Module.UTF8ToString(arrRes[1]);
        this.target = Module.UTF8ToString(arrRes[2]);
        this.score = arrRes[3];
        this.endQuery = arrRes[4];
        this.endTarget = arrRes[5];
    }

    get queryGaps() {
        return (this.query.match(/-/g) || []).length;
    }

    get targetGaps() {
        return (this.target.match(/-/g) || []).length;
    }

    get queryStart() {
        return this.endQuery + 1 - this.query.replaceAll('-', '').length + 1;
    }

    get targetStart() {
        return this.endTarget + 1 - this.target.replaceAll('-', '').length + 1;
    }

    get matches() {
        return (this.align.match(/\|/g) || []).length;
    }

    get gaps() {
        return this.queryGaps + this.targetGaps;
    }

    get alignLength() {
        return this.align.length;
    }
}