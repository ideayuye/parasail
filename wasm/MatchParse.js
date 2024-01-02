
class MatchResult {
    constructor(arrRes, originalQuery, originalTarget) {
        this.originalQuery = originalQuery;
        this.originalTarget = originalTarget;
        this.query = Module.UTF8ToString(arrRes[0]);
        this.target = Module.UTF8ToString(arrRes[2]);
        this.align = Module.UTF8ToString(arrRes[1]);
        this.score = arrRes[3];
        this.endQuery = arrRes[5];
        this.endTarget = arrRes[4];
    }

    get queryGaps() {
        return (this.query.match(/-/g) || []).length;
    }

    get targetGaps() {
        return (this.target.match(/-/g) || []).length;
    }

    get queryStart() {
        return this.endQuery + 1 - this.originalQuery.length + this.queryGaps;
    }

    get targetStart() {
        return this.endTarget + 1 - this.originalTarget.length + this.targetGaps;
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